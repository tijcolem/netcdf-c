/**
 * @file 
 * Test some illegal mode combinations
 *
 */

#include "tests.h"
#include "netcdf_par.h"

#define FILE_NAME "tst_mode.nc"

int
main()
{
   int ncid,varid;
   int retval; 

   printf("\n*** Testing illegal mode combinations\n");

   printf("*** Testing create + MPIO + fletcher32\n");
   if ((retval = nc_create_par(FILE_NAME, NC_CLOBBER|NC_NETCDF4|NC_MPIIO, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid))) ERR;
   if ((retval = nc_def_var(ncid,"whatever",NC_INT,0,NULL,&varid))) ERR;
   retval = nc_def_var_fletcher32(ncid,varid,NC_FLETCHER32);
   if(retval != NC_EINVAL) ERR;
   if ((retval = nc_abort(ncid))) ERR;

   printf("*** Testing create + MPIO + deflation\n");
   if ((retval = nc_create_par(FILE_NAME, NC_CLOBBER|NC_NETCDF4|NC_MPIIO, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid))) ERR;
   if ((retval = nc_def_var(ncid,"whatever",NC_INT,0,NULL,&varid))) ERR;
   retval = nc_def_var_deflate(ncid,varid, NC_NOSHUFFLE, 1, 1);
   if(retval != NC_EINVAL) ERR;
   if ((retval = nc_abort(ncid))) ERR;

   SUMMARIZE_ERR;

   FINAL_RESULTS;
}

