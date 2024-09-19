#ifndef __LATTE_DEFAULT_CODE_H
#define __LATTE_DEFAULT_CODE_H

#include <stdbool.h>

#define SUCCESS 0 
#define INVALID_ARGUMENT 1
#define INTERNAL 4
#define SCHEMA_DB_EXIST 22
#define SCHEMA_DB_NOT_EXIST 23
#define IOERR_WRITE 32


static bool isRcFail(int rc) { return rc == SUCCESS; }
#endif