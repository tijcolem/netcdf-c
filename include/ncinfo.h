/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef _NCINFO_H_
#define _NCINFO_H_

#include "config.h"
#include "netcdf.h"

/**
For netcdf4 files, capture state information about the following:
1. Global: netcdf library version
2. Global: hdf5 library version
3. Per file: superblock version
4. Per File: was it created by netcdf-4?
5. Per file: _NCProperties attribute
*/

/**************************************************/
/**
The goal of the properties attribute
is to tag a file with information about how it was built.  The
properties attribute is in the file in the root (group).  Its type is
always NC_CHAR (so it will work with netcdf classic) and has the form:
name=value|name=value ...  Occurrences of '|' in the value are
disallowed.  The actual name of the attribute and the set of known names
is #define'd below.

The following invariants are maintained.
1. The properties attribute is always the first global attribute in
   the root group.
2. The properties attribute can be deleted or renamed.
3. Any attempt to re-insert the properties attribute will fail
   unless there are no other global attributes.

A second invariant is: never modify the file unless absolutely necessary.
This means that even if the file is writeable and it has no props att,
do not insert one unless the user actually enables the file for writing
using nc_redef.

The occurrence of the properties attribute in the file can be
circumvented by doing things like attribute rename or delete, or using
older library versions. This should not cause any failures except
possibly to our test cases; no user-visible problems should occur.

If the file has no properties attribute, it is still readable,

In order to maintain these invariants, the library code acts as follows.

1  If a file is nc_open'd, and has no properties attribute and is not
   modified, then it will be left as is with no properties attribute.

2. If a file is nc_open'd and has a properties attribute, that attribute will
   will be parsed, but otherwise left unchanged (except as noted in step 3).

3. When using nc_create, the properties attribute will be added immediately
   when the file is created.

4. When asking for the number of global attributes, the properties attribute
   will not be included. Further for any API call that specifies the 
   attribute number, that number actually used will be modified to ignore
   the properties attribute.

The net result should be that as a rule, the properties attribute will
be the first global attribute in the root group.

One final note and justification for this complexity.  Our test cases
currently have the attribute index deeply burned into the code. So in
order to minimize test case changes, I have made the attribute be the
first and have modified the attribute numbers to make the properties
attribute generally invisible. Not sure this is a good idea and this
fact may be changed if we can (over time) modify our test cases.

*/

#define NCPROPS "_NCProperties"
#define NCPROPS_VERSION (1)
#define NCPROPSSEP  '|'
#define NCPROPS_LENGTH (8192)

/* Currently used properties */
#define NCPVERSION "version" /* Of the properties format */
#define NCPHDF5LIBVERSION "hdf5libversion"
#define NCPNCLIBVERSION "netcdflibversion"

/* Other hidden attributes */
#define ISNETCDF4ATT "_IsNetcdf4"
#define SUPERBLOCKATT "_SuperblockVersion"

/**************************************************/

struct NCPROPINFO {
    int version;
    char hdf5ver[NC_MAX_NAME+1];
    char netcdfver[NC_MAX_NAME+1];
    char text[NCPROPS_LENGTH+1]; /* Value of the NCPROPS attribute */
};

struct NCFILEINFO {
    int isnetcdf4; /* Does this file appear to have been created by netcdf4 */ 
    int superblockversion;
#if 0
    int flags;
#	define NCP_CREATE 1
#endif
    /* Following is filled from NCPROPS attribute or from global version */
    struct NCPROPINFO propattr;
};

/**************************************************/

extern struct NCPROPINFO globalpropinfo;

/*Forward*/
struct NC_HDF5_FILE_INFO;

extern int NC_fileinfo_init(void); /*libsrc4/ncinfo.c*/
extern int NC_get_fileinfo(struct NC_HDF5_FILE_INFO* info); /*libsrc4/ncinfo.c*/
extern int NC_put_propattr(struct NC_HDF5_FILE_INFO* info); /*libsrc4/ncinfo.c*/
extern int NC_isnetcdf4(struct NC_HDF5_FILE_INFO*, int*);/*libsrc4/ncinfo.c*/

/* ENABLE_FILEINFO => ENABLE_NETCDF4 */
extern int NC4_hdf5get_libversion(unsigned*,unsigned*,unsigned*);/*libsrc4/nc4hdf.c*/
extern int NC4_hdf5get_superblock(struct NC_HDF5_FILE_INFO*, int*);/*libsrc4/nc4hdf.c*/

#endif /* _NCINFO_H_ */
