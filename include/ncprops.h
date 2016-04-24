/*
 *	Copyright 1996, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef _NCPROPS_H_
#define _NCPROPS_H_

#include "config.h"
#include "netcdf.h"

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

#define NCPROPS_VERSION (1)
#define NCPROPS "_NCProperties"
#define NCPROPSSEP  '|'
#define NCPROPS_LENGTH (8192)

/* Currently used properties */
#define NCPVERSION "version" /* Of the properties format */
#define NCPHDF5LIBVERSION "hdf5libversion"
#define NCPNCLIBVERSION "netcdflibversion"

/**************************************************/
struct  NCProperties {
	    int flags;
#		define NCP_CREATE 1
	    int version; /* > 0; 0 => file has no NCPROPS attribute */
	    char hdf5libver[NC_MAX_NAME+1];
	    char netcdflibver[NC_MAX_NAME+1];
	    char text[NCPROPS_LENGTH+1];
};

extern struct NCProperties globalncproperties;

/*Forward*/
struct NC_HDF5_FILE_INFO;

extern int NC_properties_init(void);
extern int NC_get_ncproperties(struct NC_HDF5_FILE_INFO* info);
extern int NC_put_ncproperties(struct NC_HDF5_FILE_INFO* info);

/* ENABLE_PROPATTR => ENABLE_NETCDF4 */
extern int NC4_hdf5get_libversion(unsigned*,unsigned*,unsigned*);/*libsrc4/nc4hdf.c*/

#endif /* _NCPROPS_H_ */
