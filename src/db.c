#include "db.h"
#include "zmalloc/zmalloc.h"
#include "code.h"
#include "./utils.h"
#include "log/log.h"
#include "fs/dir.h"
#include "trx.h"
db* dbCreate() {
    db* d = zmalloc(sizeof(db));
    return d;
}

int dbInit(db* d,char* dbname, sds dbpath, 
        sds trx_kit_name, sds log_handler_name) {
    if (is_blank(dbname)) {
        log_error("latte_lib","Failed to init DB, name cannot be empty");
        return INVALID_ARGUMENT;
    }
    if (!dirIs(dbpath)) {
        log_error("latte_lib","Failed to init DB, path is not a directory: %s", dbpath);
        return INVALID_ARGUMENT;
    }
    trxKit* trx_kit = trxKitCreate(trx_kit_name);
}

void dbRelease(db* d) {
    zfree(d);
}