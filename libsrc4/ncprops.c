/*********************************************************************
*    Copyright 2010, UCAR/Unidata
*    See netcdf/COPYRIGHT file for copying and redistribution conditions.
* ********************************************************************/

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <hdf5.h>
#include "netcdf.h"
#include "nc4internal.h"

#define IGNORE 0

#define HDF5_MAX_NAME 1024

#define NCHECK(expr) {if((expr)!=NC_NOERR) {goto done;}}
#define HCHECK(expr) {if((expr)<0) {ncstat = NC_EHDFERR; goto done;}}

/* Global */
struct NCProperties globalncproperties;

struct NCdata {
};

int
NC_properties_init(void)
{
    int stat = NC_NOERR;
    unsigned major,minor,release;
    size_t total = 0;

    /* Build nc properties */
    memset((void*)&globalncproperties,0,sizeof(globalncproperties));
    globalncproperties.version = NCPROPS_VERSION;
    globalncproperties.netcdflibver[0] = '\0';
    globalncproperties.hdf5libver[0] = '\0';

    stat = NC4_hdf5get_libversion(&major,&minor,&release);
    if(stat) goto done;
    snprintf(globalncproperties.hdf5libver,sizeof(globalncproperties.hdf5libver),
		 "%1u.%1u.%1u",major,minor,release);
    strncpy(globalncproperties.netcdflibver,PACKAGE_VERSION,sizeof(globalncproperties.netcdflibver));
    /* Now build actual attribute text */
    total = 0;
    total += strlen(NCPVERSION);
    total += strlen("=00000000|");
    total += strlen(NCPNCLIBVERSION);
    total += strlen(globalncproperties.netcdflibver);
    total += strlen("=|");
    total += strlen(NCPHDF5LIBVERSION);
    total += strlen(globalncproperties.hdf5libver);
    total += strlen("="); /* Last pair has no trailing '|' */
    if(total >= sizeof(globalncproperties.text)) {
        fprintf(stderr,"%s size is too small\n",NCPROPS);
        goto done;
    }
    globalncproperties.text[0] = '\0';
    snprintf(globalncproperties.text,sizeof(globalncproperties.text),
		"%s=%d|%s=%s|%s=%s",
	        NCPVERSION,globalncproperties.version,
	        NCPNCLIBVERSION,globalncproperties.netcdflibver,
	        NCPHDF5LIBVERSION,globalncproperties.hdf5libver);
done:
    return stat;
}

static int
NC_properties_parse(struct NCProperties* ncprops)
{
    size_t len;
    char propdata[NCPROPS_LENGTH]; /* match nc.h struct NCProperties */
    char* p;

    ncprops->version = 0;
    ncprops->hdf5libver[0] = '\0';
    ncprops->netcdflibver[0] = '\0';

    strncpy(propdata,ncprops->text,sizeof(propdata)-1);
    propdata[sizeof(propdata)-1] = '\0';
    len = strlen(propdata);
    if(len == 0) return NC_NOERR;

    /* Walk and fill in ncprops */
    p = propdata;
    while(*p) {
	char* name = p;
	char* value = NULL;
	char* q = strchr(p,'=');
	if(q == NULL)
	    return NC_EINVAL;
	*q++ = '\0';
	value = p = q;
        q = strchr(p,NCPROPSSEP);
	if(q == NULL) q = (p+strlen(p)); else* q++ = '\0';
	p = q;
	if(name != NULL && value != NULL) {
	    if(strcmp(name,NCPVERSION) == 0) {
		int v = atoi(value);
		if(v < 0) v = 0;
		ncprops->version = v;
	    } else if(strcmp(name,NCPNCLIBVERSION) == 0)
	        strncpy(ncprops->netcdflibver,value,sizeof(ncprops->netcdflibver)-1);
	    else if(strcmp(name,NCPHDF5LIBVERSION) == 0)
	        strncpy(ncprops->hdf5libver,value,sizeof(ncprops->hdf5libver)-1);
	    /* else ignore */
	}
    }
    /* Guarantee null term */
    ncprops->netcdflibver[sizeof(ncprops->netcdflibver)-1] = '\0';
    ncprops->hdf5libver[sizeof(ncprops->hdf5libver)-1] = '\0';
    return NC_NOERR;
}

int
NC_get_ncproperties(NC_HDF5_FILE_INFO_T* info)
{
    int ncstat = NC_NOERR;
    size_t size;
    H5T_class_t t_class;	
    char text[NCPROPS_LENGTH+1];
    hid_t grp = -1;
    hid_t attid = -1;
    hid_t aspace = -1;
    hid_t atype = -1;
    hid_t ntype = -1;

    /* Get root group */
    grp = H5Gopen(info->hdfid,"/"); /* get root group */
    /* Try to extract the NCPROPS attribute */
    attid = H5Aopen_by_name(grp, ".", NCPROPS, H5P_DEFAULT, H5P_DEFAULT);
    if(attid >= 0) {
	aspace = H5Aget_space(attid); /* dimensions of attribute data */
        atype = H5Aget_type(attid);
	/* Verify that atype and size */
	t_class = H5Tget_class(atype);
	if(t_class != H5T_STRING) {ncstat = NC_ENOTATT; goto done;}
        size = H5Tget_size(atype);
	if(size != NCPROPS_LENGTH) {ncstat = NC_ENOTATT; goto done;}
        HCHECK((ntype = H5Tget_native_type(atype, H5T_DIR_ASCEND)));
        HCHECK((H5Aread(attid, ntype, text)));
	/* Try to parse text */
	strncpy(info->properties.text,text,NCPROPS_LENGTH);
	info->properties.text[NCPROPS_LENGTH-1] = '\0';
	ncstat = NC_properties_parse(&info->properties);
    }    
done:
    if(aspace >= 0) HCHECK((H5Sclose(aspace)));
    if(ntype >= 0) HCHECK((H5Tclose(ntype)));
    if(atype >= 0) HCHECK((H5Tclose(atype)));
    if(attid >= 0) HCHECK((H5Aclose(attid)));
    if(grp >= 0) HCHECK((H5Gclose(grp)));
    return ncstat;
}

int
NC_put_ncproperties(NC_HDF5_FILE_INFO_T* info)
{
    int ncstat = NC_NOERR;
    char text[NCPROPS_LENGTH+1];
    H5T_class_t t_class;	
    size_t size;
    hid_t grp = -1;
    hid_t attid = -1;
    hid_t aspace = -1;
    hid_t atype = -1;

    /* Get root group */
    grp = H5Gopen(info->hdfid,"/"); /* get root group */
    /* See if the NCPROPS attribute exists */
    attid = H5Aopen_by_name(grp, ".", NCPROPS, H5P_DEFAULT, H5P_DEFAULT);
    HCHECK((H5Aclose(attid)));
    if(attid < 0) {/* Does not exist */
        /* Create a datatype to refer to. */
        HCHECK((atype = H5Tcopy(H5T_C_S1)));
        HCHECK((H5Tset_size(atype, NCPROPS_LENGTH)));
	HCHECK((aspace = H5Screate(H5S_SCALAR)));
	HCHECK((attid = H5Acreate(grp, NCPROPS, atype, aspace, H5P_DEFAULT)));
        HCHECK((H5Awrite(attid, atype, info->properties.text)));
    }
done:
    if(aspace >= 0) HCHECK((H5Sclose(aspace)));
    if(atype >= 0) HCHECK((H5Tclose(atype)));
    if(attid >= 0) HCHECK((H5Aclose(attid)));
    if(grp >= 0) HCHECK((H5Gclose(grp)));
    return ncstat;
}

/**************************************************/
static int do_dtype(hid_t, struct NCdata*);
static int do_dset(hid_t, struct NCdata*);
static int scan_group(hid_t, struct NCdata*);
static int do_attr(hid_t, struct NCdata*);
static int scan_attrs(hid_t, struct NCdata*);
#if IGNORE
static int do_link(hid_t, char *, struct NCdata*);
static int do_plist(hid_t, struct NCdata*);
#endif

int
walk(hid_t file)
{
    int ncstat = NC_NOERR;
    hid_t    grp;
    herr_t   status;
    struct NCdata data;

    grp = H5Gopen(file,"/"); /* get root group */
    NCHECK((ncstat = scan_group(grp,&data)));
done:
    return ncstat;
}

/*
 * Process a group and all it's members
 */

static int
scan_group(hid_t gid, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    int i;
    ssize_t len;
    hsize_t nobj;
    herr_t err;
    int otype;
    hid_t grpid, typeid, dsid;
    char group_name[HDF5_MAX_NAME];
    char memb_name[HDF5_MAX_NAME];

    /*
     * Information about the group:
     *  Name and attributes
     *  Other info., not shown here: number of links, object id
     */
    len = H5Iget_name(gid, group_name, HDF5_MAX_NAME);

    /*
     *  process the attributes of the group, if any.
     */
    NCHECK((ncstat = scan_attrs(gid,data)));

    /*
     *  Get all the members of the groups, one at a time.
     */
    err = H5Gget_num_objs(gid, &nobj);
    for(i = 0; i < nobj; i++) {
        /*
         *  For each object in the group, get the name and
         *   what type of object it is.
         */
        len = H5Gget_objname_by_idx(gid,(hsize_t)i,memb_name,(size_t)HDF5_MAX_NAME );
        otype =  H5Gget_objtype_by_idx(gid,(size_t)i );
	/*
         * process each object according to its type
         */
        switch(otype) {
        case H5G_LINK:
#if IGNORE
            NCHECK((ncstat = do_link(gid,memb_name,data)));
#endif
            break;
        case H5G_GROUP:
            grpid = H5Gopen(gid,memb_name);
            NCHECK((ncstat = scan_group(grpid,data)));
            H5Gclose(grpid);
            break;
        case H5G_DATASET:
            dsid = H5Dopen(gid,memb_name);
            NCHECK((ncstat = do_dset(dsid,data)));
            H5Dclose(dsid);
            break;
        case H5G_TYPE:
            typeid = H5Topen(gid,memb_name);
            NCHECK((ncstat = do_dtype(typeid,data)));
            H5Tclose(typeid);
            break;
        default:
	    abort();
            break;
        }
    }
done:
    return ncstat;
}

/*
 * Retrieve information about a dataset.
 *
 * Many other possible actions.
 *
 * This example does not read the data of the dataset.
 */
static int
do_dset(hid_t did, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    hid_t tid;
    hid_t pid;
    hid_t sid;
    hsize_t size;
    char ds_name[HDF5_MAX_NAME];

    /*
     * Information about the group:
     * Name and attributes
     *
     * Other info., not shown here: number of links, object id
     */
    H5Iget_name(did, ds_name, HDF5_MAX_NAME);

    /*
     * process the attributes of the dataset, if any.
     */
    NCHECK((ncstat = scan_attrs(did,data)));

    /*
     * Get dataset information: dataspace, data type
     */
    sid = H5Dget_space(did); /* the dimensions of the dataset(not shown) */
    tid = H5Dget_type(did);
    NCHECK((ncstat = do_dtype(tid,data)));

    /*
     * Retrieve and analyse the dataset properties
     */
#if IGNORE
    pid = H5Dget_create_plist(did); /* get creation property list */
    NCHECK((ncstat = do_plist(pid,data)));
    size = H5Dget_storage_size(did);
    H5Pclose(pid);
#endif

    /*
     * The datatype and dataspace can be used to read all or
     * part of the data. (Not shown in this example.)
     */
    H5Tclose(tid);
    H5Sclose(sid);
done:
    return ncstat;
}

/*
 * Analyze a data type description
 */
static int
do_dtype(hid_t tid, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    H5T_class_t t_class;

    t_class = H5Tget_class(tid);
    if(t_class < 0) {/* Invalid datatype*/
	ncstat = NC_EHDFERR;
	goto done;
    } else {
        /*
         * Each class has specific properties that can be
         * retrieved, e.g., size, byte order, exponent, etc.
         */
	if(t_class == H5T_INTEGER) {
            /* display size, signed, endianess, etc. */
        } else if(t_class == H5T_FLOAT) {
            /* display size, endianess, exponennt, etc. */
        } else if(t_class == H5T_STRING) {
            /* display size, padding, termination, etc. */
        } else if(t_class == H5T_BITFIELD) {
            /* display size, label, etc. */
        } else if(t_class == H5T_OPAQUE) {
            /* display size, etc. */
        } else if(t_class == H5T_COMPOUND) {
            /* recursively display each member: field name, type */
        } else if(t_class == H5T_ARRAY) {
            /* display dimensions, base type */
        } else if(t_class == H5T_ENUM) {
            /* display elements: name, value  */
        } else {
           /* eg. Object Reference, ...and so on ... */
        }
    }
done:
    return ncstat;
}

/*
 * Run through all the attributes of a dataset or group.
 * This is similar to iterating through a group.
 */
static int
scan_attrs(hid_t oid, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    int na;
    hid_t aid;
    int i;

    na = H5Aget_num_attrs(oid);

    for(i = 0; i < na; i++) {
        aid =  H5Aopen_idx(oid,(unsigned int)i);
        NCHECK((ncstat = do_attr(aid,data)));
        H5Aclose(aid);
    }
done:
    return ncstat;
}

/*
 * Process one attribute.
 * This is similar to the information about a dataset.
 */
static int
do_attr(hid_t aid, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    ssize_t len;
    hid_t atype;
    hid_t aspace;
    char buf[HDF5_MAX_NAME];

    /*
     * Get the name of the attribute.
     */
    len = H5Aget_name(aid, HDF5_MAX_NAME, buf);

    /*
     * Get attribute information: dataspace, data type
     */
    aspace = H5Aget_space(aid); /* the dimensions of the attribute data */

    atype = H5Aget_type(aid);
    NCHECK((ncstat = do_dtype(atype,data)));

    /*
     * The datatype and dataspace can be used to read all or
     * part of the data. (Not shown in this example.)
     */
     /* ... read data with H5Aread, write with H5Awrite, etc. */

    H5Tclose(atype);
    H5Sclose(aspace);
done:
    return ncstat;
}

#if IGNORE
/*
 * Analyze a symbolic link
 *
 * The main thing you can do with a link is find out
 * what it points to.
 */
static int
do_link(hid_t gid, char *name, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    herr_t status;
    char target[HDF5_MAX_NAME];

    HCHECK((status = H5Gget_linkval(gid, name, HDF5_MAX_NAME, target)));
done:
    return ncstat;
}

/*
 *  Example of information that can be read from a Dataset Creation
 *  Property List.
 *
 *  There are many other possibilities, and there are other property
 *  lists.
 */
static int
do_plist(hid_t pid, struct NCdata* data)
{
    int ncstat = NC_NOERR;
    hsize_t chunk_dims_out[2];
    int rank_chunk;
    int nfilters;
    H5Z_filter_t filtn;
    int i;
    unsigned int  filt_flags, filt_conf;
    size_t cd_nelmts;
    unsigned int cd_values[32] ;
    char f_name[HDF5_MAX_NAME];
    H5D_fill_time_t ft;
    H5D_alloc_time_t at;
    H5D_fill_value_t fvstatus;
    unsigned int szip_options_mask;
    unsigned int szip_pixels_per_block;

    /* zillions of things might be on the plist */
    /* here are a few... */

    /*
     * get chunking information: rank and dimensions.
     *
     * For other layouts, would get the relevant information.
     */
    if(H5D_CHUNKED == H5Pget_layout(pid)){
        rank_chunk = H5Pget_chunk(pid, 2, chunk_dims_out);
#if 0
        printf("chunk rank %d, dimensions %lu x %lu\n", rank_chunk,
         (unsigned long)(chunk_dims_out[0]),
         (unsigned long)(chunk_dims_out[1]));
#endif
    } /* else if contiguous, etc. */

    /*
     * Get optional filters, if any.
     *
     * This include optional checksum and compression methods.
     */

    nfilters = H5Pget_nfilters(pid);
    for(i = 0; i < nfilters; i++)
    {
        /* For each filter, get
         *  filter ID
         *  filter specific parameters
         */
        cd_nelmts = 32;
        filtn = H5Pget_filter(pid,(unsigned)i,
            &filt_flags, &cd_nelmts, cd_values,
            (size_t)HDF5_MAX_NAME, f_name, &filt_conf);
        /*
         * These are the predefined filters
         */
        switch(filtn) {
            case H5Z_FILTER_DEFLATE: /* AKA GZIP compression */
#if 0
                printf("DEFLATE level = %d\n", cd_values[0]);
#endif
                break;
            case H5Z_FILTER_SHUFFLE:
#if 0
                printf("SHUFFLE\n"); /* no parms */
#endif
                break;
            case H5Z_FILTER_FLETCHER32:
#if 0
                printf("FLETCHER32\n"); /* Error Detection Code */
#endif
                break;
            case H5Z_FILTER_SZIP:
#if 0
                szip_options_mask=cd_values[0];
                szip_pixels_per_block=cd_values[1];

                printf("SZIP COMPRESSION: ");
                printf("PIXELS_PER_BLOCK %d\n",
                    szip_pixels_per_block);
                 /* print SZIP options mask, etc. */
#endif
                break;
            default:
		ncstat = NC_EHDFERR; /*UNKNOWN_FILTER*/
		goto done;
                break;
        }
    }

    /*
     * Get the fill value information:
     *  - when to allocate space on disk
     *  - when to fill on disk
     *  - value to fill, if any
     */

    H5Pget_alloc_time(pid, &at);
    switch(at)
    {
    case H5D_ALLOC_TIME_EARLY:
        break;
    case H5D_ALLOC_TIME_INCR:
        break;
    case H5D_ALLOC_TIME_LATE:
        break;
    default:
        break;
    }

    H5Pget_fill_time(pid, &ft);
    switch( ft) {
    case H5D_FILL_TIME_ALLOC:
        break;
    case H5D_FILL_TIME_NEVER:
        break;
    case H5D_FILL_TIME_IFSET:
        break;
    default:
        break;
    }

    H5Pfill_value_defined(pid, &fvstatus);

    if(fvstatus == H5D_FILL_VALUE_UNDEFINED) {
        //No fill value defined, will use default
    } else {
        /* Read the fill value with H5Pget_fill_value.
         * Fill value is the same data type as the dataset.
         *(details not shown)
         **/
    }

    /* ... and so on for other dataset properties ... */

done:
    return ncstat;
}
#endif

