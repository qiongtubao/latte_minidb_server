#ifndef __LATTE_SESSION_H
#define __LATTE_SESSION_H
#include "db.h"
typedef struct session {
    struct Db* db;
    struct Trx* trx;
    struct SessionEvent* current_request;
    int trx_multi_operation_mode;
    int sql_debug;
    int used_chunk_mode;
    struct ExecutionMode* execution_mode;
} session;

struct session* default_session();

#endif
