#ifndef __LATTE_DEFAULT_CODE_H
#define __LATTE_DEFAULT_CODE_H

#include <stdbool.h>

#define SUCCESS 0 
#define INVALID_ARGUMENT 1
#define INTERNAL 4
#define NOMEM 5
#define BUFFERPOOL_OPEN 11
#define BUFFERPOOL_NOBUF 12
#define RECORD_OPENNED 14
#define SCHEMA_DB_EXIST 22
#define SCHEMA_DB_NOT_EXIST 23
#define IOERR_READ 31
#define IOERR_WRITE 32
#define IOERR_ACCESS 33
#define IOERR_OPEN 34
#define IOERR_CLOSE 35
#define IOERR_SEEK 36
#define IOERR_TOO_LONG 37
#define LOCKED_UNLOCK 39
#define LOG_ENTRY_INVALID 55



static bool isRcFail(int rc) { return rc != SUCCESS; }
#endif