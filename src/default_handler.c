#include "default_handler.h"
#include "zmalloc/zmalloc.h"
#include <errno.h>
#include "log/log.h"
#include "utils/error.h"
#include "fs/dir.h"
#include "default_handler.h"
#include "session.h"
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include "code.h"
#include "./utils.h"
#include "dict/dict_plugins.h"
#include "./log.h"
static dict_func_t openedDbDict = {
    dict_char_hash,
    NULL,
    NULL,
    dict_char_key_compare,
    dict_sds_destructor,
    NULL,
    NULL
};
db_handler_t* db_handler_new() {
    db_handler_t* handler = zmalloc(sizeof(db_handler_t));
    handler->base_dir = NULL;
    handler->db_dir = NULL;
    handler->log_handler_name = NULL;
    handler->trx_kit_name = NULL;
    handler->opened_dbs = dict_new(&openedDbDict);
    return handler;
}



int handler_create_db(db_handler_t* handler, const char* dbname) { //创建db
    if (NULL == dbname ||
        is_blank(dbname)) {
        log_warn("latte_lib", "Invalid db name");
        return INVALID_ARGUMENT;
    }
    sds dbpath = sds_cat_fmt(sds_empty(), "%s/%s", handler->db_dir, dbname);
    if (dirIs(dbpath)) {
        log_warn("latte_lib", "Db already exists: %s", dbname);
        return SCHEMA_DB_EXIST;
    }
    int ec = 0755;
    if (!isOk(dirCreateRecursive(dbpath, ec))) {
        log_error("latte_lib", "Create db fail: %s. error=%s", dbpath, strerror(errno));
        return IOERR_WRITE;
    }
    return SUCCESS;
}

int drop_db(db_handler_t* handler, const char* dbname) {
    return INTERNAL;
}

int handler_open_db(db_handler_t* handler, const char* dbname) {
    if (NULL == dbname || is_blank(dbname)) {
        miniDBServerLog(LOG_ERROR, "Invalid db name");
        return INVALID_ARGUMENT;
    }
    dict_entry_t* entry = dict_find(handler->opened_dbs, dbname);
    if (entry != NULL) {
        miniDBServerLog(LOG_ERROR, "opened db (%s)", dbname);
        return SUCCESS;
    }

    sds dbpath = sds_cat_fmt(sds_empty(), "%s/%s", handler->db_dir, dbname);
    if (!dirIs(dbpath)) {
        return SCHEMA_DB_NOT_EXIST;
    }
    db_t* d = db_new();
    int ret = SUCCESS;
    if ((ret = db_init(d, dbname, dbpath, 
        handler->trx_kit_name, handler->log_handler_name)) != SUCCESS) {
        miniDBServerLog(LOG_ERROR, "Failed to open db: %s. ", dbname);
        db_delete(d);
    } else {
        dict_add(handler->opened_dbs, sds_new(dbname), d);
    }
    return ret;
}

int db_handler_init(db_handler_t* handler, const char* base_dir, const char* trx_kit_name, const char *log_handler_name) {
    int rc = 0;

    sds db_dir = sds_cat_fmt(sds_empty(),"%s/db", base_dir);
    if (!dirIs(db_dir) && 
        !isOk(dirCreateRecursive(db_dir, 0755))) {
        miniDBServerLog(LOG_ERROR, "Cannot access base dir: %s. msg=%d:%s", db_dir, errno, strerror(errno));
            return INTERNAL;
    }
    
    handler->base_dir = sds_new(base_dir);
    handler->db_dir = db_dir;
    handler->trx_kit_name = sds_new(trx_kit_name);
    handler->log_handler_name = sds_new(log_handler_name);

    const char* sys_db = "sys";
    int ret = handler_create_db(handler, sys_db); //创建sys_db数据库 只是创建文件夹
    if (ret != 0 
        && ret != SCHEMA_DB_EXIST) {
        miniDBServerLog(LOG_ERROR, "Failed to open system db. rc=%d", ret);
        return ret;
    }
    miniDBServerLog(LOG_INFO, "handler_open_db before");
    ret = handler_open_db(handler, sys_db);
    miniDBServerLog(LOG_INFO, "handler_open_db");
    if (ret != SUCCESS) {
        miniDBServerLog(LOG_ERROR, "Failed to open system db. rc=%d", ret);
        return ret;
    }
    // 还不知道session什么用， 应该放到哪里
    // struct session* dsession = default_session();
    // struct db_t* d = find_db(handler, sys_db);
    // session_set_current_db(dsession, d);
    // log_info("latte_lib", "Default handler init with %s success", base_dir);
    return rc;
}


db_t* handler_find_db(db_handler_t* handler,const char* dbname) {
    dict_entry_t* d = dict_find(handler->opened_dbs, dbname);
    return  dict_get_val(d);
}