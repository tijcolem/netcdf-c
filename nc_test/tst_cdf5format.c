/* This is part of the netCDF package.
   Copyright 2005 University Corporation for Atmospheric Research/Unidata
   See COPYRIGHT file for conditions of use.

   Test fix of bug involving creation of a file with pnetcdf APIs,
   then opening and modifying the file with netcdf.

   Author: Wei-keng Liao.
*/

/*
Goal is to verify that cdf-5 code is writing data that can
be read by pnetcdf and vice-versa.
*/

/*
    Compile:
        mpicc -g -o tst_cdf5format tst_cdf5format.c -lnetcdf -lcurl -lhdf5_hl -lhdf5 -lpnetcdf -lz -lm
    Run:
        nc_pnc
*/

#include <nc_tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>
#include <assert.h>

#define NVARS 6
#define NX    5
#define FILENAME "tst_pnetcdf.nc"

/*
Given ncid,
write meta-data and data
*/

void
write(int ncid, int parallel)
{
    int i, j, rank, nprocs, ncid, cmode, varid[NVARS], dimid[2], *buf;
    int err = 0;
    char str[32];
    size_t start[2], count[2];

    /* define dimension */
    if (nc_def_dim(ncid, "Y", NC_UNLIMITED, &dimid[0])) ERR;
    if (nc_def_dim(ncid, "X", NX,           &dimid[1])) ERR;

    /* Odd numbers are fixed variables, even numbers are record variables */
    for (i=0; i<NVARS; i++) {
        if (i%2) {
            sprintf(str,"fixed_var_%d",i);
            if (nc_def_var(ncid, str, NC_INT, 1, dimid+1, &varid[i])) ERR;
        }
        else {
            sprintf(str,"record_var_%d",i);
            if (nc_def_var(ncid, str, NC_INT, 2, dimid, &varid[i])) ERR;
        }
    }
    if (nc_enddef(ncid)) ERR;

    for (i=0; i<NVARS; i++) {
	if(parallel) {
            /* Note NC_INDEPENDENT is the default */
            if (nc_var_par_access(ncid, varid[i], NC_INDEPENDENT)) ERR;
	}
    }

    /* write all variables */
    buf = (int*) malloc(NX * sizeof(int));
    for (i=0; i<NVARS; i++) {
        for (j=0; j<NX; j++) buf[j] = i*10 + j;
        if (i%2) {
            start[0] = 0; count[0] = NX;
            if (nc_put_vara_int(ncid, varid[i], start, count, buf)) ERR;
        }
        else {
            start[0] = 0; start[1] = 0;
            count[0] = 1; count[1] = NX;
            if (nc_put_vara_int(ncid, varid[i], start, count, buf)) ERR;
        }
    }
}

void
extend(int ncid)
{
    if (nc_redef(ncid)) ERR;
    /* add attributes to make header grow */
    for (i=0; i<NVARS; i++) {
        sprintf(str, "annotation_for_var_%d",i);
        if (nc_put_att_text(ncid, varid[i], "text_attr", strlen(str), str)) ERR;
    }
    if (nc_enddef(ncid)) ERR;
}

void
read(int ncid)
{
    int i, j, rank, nprocs, ncid, cmode, varid[NVARS], dimid[2], *buf;
    int err = 0;
    char str[32];
    size_t start[2], count[2];

    /* read variables and check their contents */
    for (i=0; i<NVARS; i++) {
        for (j=0; j<NX; j++) buf[j] = -1;
        if (i%2) {
            start[0] = 0; count[0] = NX;
            if (nc_get_var_int(ncid, varid[i], buf)) ERR;
            for (j=0; j<NX; j++)
                if (buf[j] != i*10 + j)
                    printf("unexpected read value var i=%d buf[j=%d]=%d should be %d\n",i,j,buf[j],i*10+j);
        }
        else {
            start[0] = 0; start[1] = 0;
            count[0] = 1; count[1] = NX;
            if (nc_get_vara_int(ncid, varid[i], start, count, buf)) ERR;
            for (j=0; j<NX; j++)
                if (buf[j] != i*10+j)
                    printf("unexpected read value var i=%d buf[j=%d]=%d should be %d\n",i,j,buf[j],i*10+j);
        }
    }
}


int main(int argc, char* argv[])
{
/*
    int i, j, rank, nprocs, ncid, cmode, varid[NVARS], dimid[2], *buf;
    int err = 0;
    char str[32];
    size_t start[2], count[2];
*/
    MPI_Comm comm=MPI_COMM_SELF;
    MPI_Info info=MPI_INFO_NULL;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (nprocs > 1 && rank == 0)
        printf("This test program is intended to run on ONE process\n");
    if (rank > 0) goto fn_exit;

#ifdef DISABLE_PNETCDF_ALIGNMENT
    MPI_Info_create(&info);
    MPI_Info_set(info, "nc_header_align_size", "1");
    MPI_Info_set(info, "nc_var_align_size",    "1");
#endif

    /* pnetcdf->cdf5 */
    printf("\nWrite using PNETCDF; Read using CDF5\n");
    
    cmode = NC_PNETCDF | NC_CLOBBER;
    if (nc_create_par(FILENAME, cmode, comm, info, &ncid)) ERR_RET;
    write(ncid,1);
    if (nc_close(ncid)) ERR;
    /* re-open the file with netCDF (parallel) and enter define mode */
    if (nc_open_par(FILENAME, NC_WRITE|NC_PNETCDF, comm, info, &ncid)) ERR_RET;
    extend(ncid);
    if (nc_close(ncid)) ERR;

    cmode = NC_CDF5 | NC_NOCLOBBER;
    if (nc_open(FILENAME, cmode, &ncid)) ERR_RET;
    read(ncid);
    if (nc_close(ncid)) ERR;

    unlink(FILENAME);

    /* cdf5->pnetcdf */
    printf("\nWrite using CDF-5; Read using PNETCDF\n");
    cmode = NC_CDF5 | NC_CLOBBER;
    if (nc_create(FILENAME, cmode, &ncid)) ERR_RET;
    write(ncid,0);
    if (nc_close(ncid)) ERR;
    /* re-open the file with netCDF (parallel) and enter define mode */
    if (nc_open(FILENAME, NC_WRITE|NC_CDF5, &ncid)) ERR_RET;
    extend(ncid);
    if (nc_close(ncid)) ERR;

    cmode = NC_PNETCDF | NC_NOCLOBBER;
    if (nc_open_par(FILENAME, cmode, comm, info, &ncid)) ERR_RET;
    read(ncid);
    if (nc_close(ncid)) ERR;

    if (info != MPI_INFO_NULL) MPI_Info_free(&info);

fn_exit:
    MPI_Finalize();
    SUMMARIZE_ERR;
    FINAL_RESULTS;
    return 0;
}

*/
