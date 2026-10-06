#ifndef DS_COMMON_H
#define DS_COMMON_H

/* Status codes returned by every libds operation that can fail. */
typedef enum {
    DS_OK           =  0,
    DS_ERR_NOMEM    = -1, /* allocation failed; the container is unchanged */
    DS_ERR_INVALID  = -2, /* NULL container or argument                    */
    DS_ERR_RANGE    = -3, /* index out of range                            */
    DS_ERR_EMPTY    = -4, /* pop from an empty container                   */
    DS_ERR_NOTFOUND = -5  /* key not present                               */
} ds_status;

/* qsort-style comparator: <0, 0 or >0. */
typedef int (*ds_cmp_fn)(const void *a, const void *b);

const char *ds_strerror(int status);

#endif /* DS_COMMON_H */
