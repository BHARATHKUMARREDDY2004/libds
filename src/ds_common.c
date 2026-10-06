#include "ds_common.h"

const char *ds_strerror(int status)
{
    switch (status) {
    case DS_OK:           return "ok";
    case DS_ERR_NOMEM:    return "out of memory";
    case DS_ERR_INVALID:  return "invalid argument";
    case DS_ERR_RANGE:    return "index out of range";
    case DS_ERR_EMPTY:    return "container is empty";
    case DS_ERR_NOTFOUND: return "not found";
    default:              return "unknown error";
    }
}
