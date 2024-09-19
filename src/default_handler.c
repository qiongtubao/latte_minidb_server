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
dbHandler* createDefaultHandler() {
    dbHandler* handler = zmalloc(sizeof(dbHandler));
    return handler;
}


bool is_blank(const char *s) {
    // 如果字符串指针为 nullptr，则认为该字符串为空白
    if (s == NULL) {
        return true;
    }
    
    // 遍历字符串的每一个字符
    while (*s != '\0') {
        // 如果当前字符不是空白字符，则返回 false
        if (!isspace((unsigned char)*s)) {
            return false;
        }
        s++; // 移动到下一个字符
    }
    
    // 如果所有字符都是空白字符，则返回 true
    return true;
}

#define SUCCESS 0 
#define INVALID_ARGUMENT 1
#define INTERNAL 4
#define SCHEMA_DB_EXIST 22
#define IOERR_WRITE 32
int create_db(dbHandler* handler, const char* dbname) { //创建db
    if (NULL == dbname ||
        is_blank(dbname)) {
        log_warn("latte_lib", "Invalid db name");
        return INVALID_ARGUMENT;
    }
    sds dbpath = sdscatfmt(sdsempty(), "%s/%s", handler->db_dir, dbname);
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


int init_default_handler(dbHandler* handler, const char* base_dir, const char* trx_kit_name, const char *log_handler_name) {
    int rc = 0;
    sds db_dir = sdsnew(base_dir);
    db_dir = sdscatfmt("%s/db", db_dir);
    if (!dirIs(db_dir) && 
        !dirCreateRecursive(db_dir, 0755)) {
        log_error("latte_lib", "Cannot access base dir: %s. msg=%d:%s", db_dir, errno, strerror(errno));
            return INTERNAL;
    }
    const char* sys_db = "sys";
    int ret = create_db(handler, sys_db); //创建sys_db数据库 只是创建文件夹
    if (ret != 0 
        && ret != SCHEMA_DB_EXIST) {
        log_error("latte_lib", "Failed to open system db. rc=%d", ret);
        return ret;
    }
    struct session* dsession = default_session();
    session_set_current_db(default_session, sys_db);
    log_info("latte_lib", "Default handler init with %s success", base_dir);


    return rc;
}


struct db* find_db(dbHandler* handler,const char* dbname) {
    dictEntry* d = dictFind(handler->opened_dbs, dbname);
    return  dictGetVal(d);
}