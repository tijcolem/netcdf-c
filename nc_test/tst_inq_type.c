/* This is part of the netCDF package. Copyright 2016 University
   Corporation for Atmospheric Research/Unidata See COPYRIGHT file for
   conditions of use. See www.unidata.ucar.edu for more info.

   Test nc_inq_type

   Added in support of https://github.com/Unidata/netcdf/issues/240

*/

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include <nc_tests.h>
#include <netcdf.h>

#ifdef USE_PNETCDF
#include <netcdf_par.h>
#endif

#define FILE_NAME "tst_inq_type.nc"

void
check_err(const int stat, const int line, const char *file) {
   if (stat != NC_NOERR) {
      (void)fprintf(stderr,"line %d of %s: %s\n", line, file, nc_strerror(stat));
      fflush(stderr);
      exit(1);
   }
}

int test_type(int type, char* tstring) {

  printf("\t* Testing Type %s:\t",tstring);
  if(nc_inq_type(0,type,NULL,NULL)) ERR;
  else printf("success.\n");

  return 0;
}



int main(int argc, char **argv) {

  int ncid=0;

  printf("\n* Testing nc_inq_type\n");

  if(nc_create(FILE_NAME,NC_WRITE,&ncid)) ERR;

  test_type(NC_BYTE,"NC_BYTE");
  test_type(NC_CHAR,"NC_CHAR");
  test_type(NC_SHORT,"NC_SHORT");
  test_type(NC_INT,"NC_INT");
  test_type(NC_LONG,"NC_LONG");
  test_type(NC_FLOAT,"NC_FLOAT");
  test_type(NC_DOUBLE,"NC_DOUBLE");
  test_type(NC_UBYTE,"NC_UBYTE");
  test_type(NC_USHORT,"NC_USHORT");
  test_type(NC_UINT,"NC_UINT");
  test_type(NC_INT64,"NC_INT64");
  test_type(NC_UINT64,"NC_UINT64");
  test_type(NC_STRING,"NC_STRING");


  printf("* Finished.\n");
  if(nc_close(ncid)) ERR;

  SUMMARIZE_ERR;
  FINAL_RESULTS;
}
