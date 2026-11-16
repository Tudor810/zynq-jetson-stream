#include "errors.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

const char *err_str(int code)
{
    switch (code) {
    case ERR_OK:         return "success";
    case ERR_ARGS:       return "invalid arguments";
    case ERR_NOMEM:      return "out of memory";
    case ERR_FILE_OPEN:  return "cannot open file";
    case ERR_FILE_READ:  return "cannot read file";
    case ERR_FILE_WRITE: return "cannot write file";
    default:             return "unknown error";
    }
}

void err_print(const char *context, int code)
{
    if (errno != 0)
        fprintf(stderr, "%s: %s (%s)\n", context, err_str(code), strerror(errno));
    else
        fprintf(stderr, "%s: %s\n", context, err_str(code));
}
