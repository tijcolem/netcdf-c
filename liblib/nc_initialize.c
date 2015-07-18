/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#include "config.h"

#ifdef USE_PARALLEL
#include <mpi.h>
#endif

#include "ncdispatch.h"

extern int NC3_initialize(void);
extern int NC3_finalize(void);

#ifdef USE_NETCDF4
extern int NC4_initialize(void);
extern int NC4_finalize(void);
#endif

#ifdef USE_DAP
extern int NCD2_initialize(void);
extern int NCD2_finalize(void);
#endif

#ifdef USE_PNETCDF
extern int NCP5_initialize(void);
extern int NCP5_finalize(void);
#endif

int NC_argc = 1;
char* NC_argv[] = {"nc_initialize",NULL};

int NC_initialized = 0;
int NC_finalized = 1;

/**
This procedure invokes all defined
initializers, and there is an initializer
for every known dispatch table.
So if you modify the format of NC_Dispatch,
then you need to fix it everywhere.
It also initializes appropriate external libraries.
*/

int
nc_initialize()
{
    int stat = NC_NOERR;
#ifdef USE_PARALLEL
    char** argv = nc_argv;
    int argc = nc_argc;
#endif

    if(NC_initialized) return NC_NOERR;
    NC_initialized = 1;
    NC_finalized = 0;

    /* Init external libraries */
#ifdef USE_PARALLEL
    {
	MPI_finalize(argc,argv);
    }
#endif

    /* Initialize each active protocol */

    if((stat = NCSUBSTRATE_initialize())) return stat;

    if((stat = NC3_initialize())) return stat;

#ifdef USE_DAP
    if((stat = NCD2_initialize())) return stat;
#endif

#ifdef USE_PNETCDF
    if((stat = NCP5_initialize())) return stat;
#endif

#ifdef USE_NETCDF4
    if((stat = NC4_initialize())) return stat;
#endif /* USE_NETCDF4 */

    /* Allow libdispatch to do initialization; also initializes substrate*/
    if((stat = NCDISPATCH_initialize())) return stat;

    return NC_NOERR;
}

/**
This procedure invokes all defined
finalizers, and there should be one
for every known dispatch table.
So if you modify the format of NC_Dispatch,
then you need to fix it everywhere.
It also finalizes appropriate external libraries.
*/

int
nc_finalize(void)
{
    int stat = NC_NOERR;

    if(NC_finalized) return NC_NOERR;
    NC_initialized = 0;
    NC_finalized = 1;

    /* Finalize each active protocol */

#ifdef USE_DAP
    if((stat = NCD2_finalize())) return stat;
#endif

#ifdef USE_PNETCDF
    if((stat = NCP5_finalize())) return stat;
#endif

#ifdef USE_NETCDF4
    if((stat = NC4_finalize())) return stat;
#endif /* USE_NETCDF4 */

    if((stat = NC3_finalize())) return stat;

    /* Allow libdispatch to do finalization; also finalizes substrate */
    if((stat = NCDISPATCH_finalize())) return stat;

    /* Finalize external libraries */
#ifdef USE_PARALLEL
    MPI_Finalize();
#endif

    return NC_NOERR;
}


