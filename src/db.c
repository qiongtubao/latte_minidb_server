#include "db.h"
#include "zmalloc/zmalloc.h"
#include "code.h"
#include "./utils.h"
#include "log/log.h"
#include "fs/dir.h"
#include "trx.h"
#include "log.h"
#include "disk_buffer_pool.h"
#include "double_write_buffer.h"
#include "log_handler.h"
#include <unistd.h>
#include "iterator/iterator.h"
#include <assert.h>
#include "table.h"
#include "log_replayer_integrated.h"
#include <strings.h>
#include <string.h>

db* dbCreate() {
    db* d = zmalloc(sizeof(db));
    return d;
}

int dbMetaInit(db* d) {
    sds db_meta_file_path = sdscatfmt(sdsempty(), "%s/%s.db", d->path, d->name);
    if (!file_exists(db_meta_file_path)) {
        d->check_point_lsn = 0;
        miniDBServerLog(LOG_INFO,"Db meta file not exist. db=%s, file=%s", d->name, db_meta_file_path);
        return SUCCESS;
    }
    int fd = open(db_meta_file_path, O_RDONLY);
    if (fd < 0) {
        miniDBServerLog(LOG_ERROR,"Failed to open db meta file. db=%s, file=%s, errno=%s", 
              d->name, db_meta_file_path, strerror(errno));
        return IOERR_READ; 
    }
    int rc = SUCCESS;
    //读取meta文件
    char buffer[1024];
    int n = read(fd, buffer, sizeof(buffer));
    if (n < 0) {
        miniDBServerLog(LOG_ERROR,"Failed to read db meta file. db=%s, file=%s, errno=%s", 
              d->name, db_meta_file_path, strerror(errno));
        rc = IOERR_READ; 
    } else {
        if (n >= sizeof(buffer)) {
            miniDBServerLog(LOG_ERROR,"Db meta file is too large. db=%s, file=%s, buffer size=%ld", 
              d->name, db_meta_file_path, sizeof(buffer));
            rc = IOERR_TOO_LONG;
        } else {
            buffer[n] = '\0';
            d->check_point_lsn = atoll(buffer);  // 当前元数据就这一个数字
            miniDBServerLog("Db meta file is too large. db=%s, file=%s, buffer size=%ld", 
              d->name, db_meta_file_path, sizeof(buffer));
        }
    }
    close(fd);
    return rc;
}

static const char *TABLE_META_FILE_PATTERN = ".*\\.table$";
int db_open_all_tables(db* d) {
    Iterator* iterator =  dir_scan_file(d->path, TABLE_META_FILE_PATTERN);
    if (iterator == NULL) {
        miniDBServerLog(LOG_ERROR, "Failed to list table meta files under %s.", d->path);
        return IOERR_READ;
    }
    int rc = SUCCESS;
    while (iteratorHasNext(iterator)) {
        char* filename = iteratorNext(iterator);
        table* t = tableCreate();
        rc = tableOpen(t, d, d->path, filename);
        if (isRcFail(rc)) {
            miniDBServerLog(LOG_ERROR, "Failed to open table. filename=%s", filename);
            tableRelease(t);
            goto fail;
        }
        dictEntry* entry = dictFind(d->opened_tables, t->table_meta->name);
        if (entry != NULL) {
            miniDBServerLog(LOG_ERROR, "Duplicate table with difference file name. table=%s, the other filename=%s",
                t->table_meta->name, filename);
            tableRelease(t);
            goto fail;
        }

        if (t->table_meta->table_id >= d->next_table_id) {
            d->next_table_id = t->table_meta->table_id + 1;
        }
        assert(DICT_OK == dictAdd(d->opened_tables, t->table_meta->name, t));

        miniDBServerLog(LOG_INFO, "Open table: %s, file: %s", t->table_meta->name, filename);
    }
    miniDBServerLog(LOG_INFO, "All table have been opened. num=%d", dictSize(d->opened_tables));
fail:
    iteratorRelease(iterator);
    return rc;  
}

int init_dblwr_buffer(db* d) {
    diskDoubleWriteBuffer* dblwr_buffer = d->buffer_pool_manager->dblwr_buffer;
    int rc = dblwr_buffer_recover(dblwr_buffer);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "fail to recover in dblwr buffer");
        return rc;
    }
    return SUCCESS;
}

int dbRecover(db* d) {
    miniDBServerLog(LOG_TRACE, "db recover begin. check_point_lsn=%d", d->check_point_lsn);

    logReplayer* trx_log_replayer = d->trx_kit->create_log_replayer(d->trx_kit, d, d->log_handler);
    if (trx_log_replayer == NULL) {
        miniDBServerLog(LOG_ERROR, "Failed to create trx log replayer.");
        return INTERNAL;
    }

    integratedLogReplayer* log_replayer = integratedLogReplayerCreate(d->buffer_pool_manager, trx_log_replayer);
    int rc = d->log_handler->replay(d->log_handler, log_replayer, d->check_point_lsn);

    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to replay log. rc=%d", (rc));
        return rc;
    }

    rc = d->log_handler->start(d->log_handler);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to start log handler. rc=%d", rc);
        return rc;
    }

    rc = log_replayer->supper.on_done(log_replayer);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN,"failed to on_done. rc=%d", (rc));
        return rc;
    }

    miniDBServerLog(LOG_INFO, "Successfully recover db. db=%s checkpoint_lsn=%d", d->name, d->check_point_lsn);
    return rc;
}

int dbInit(db* d,char* dbname, sds dbpath, 
        sds trx_kit_name, sds log_handler_name) {
    if (is_blank(dbname)) {
        miniDBServerLog(LOG_ERROR,"Failed to init DB, name cannot be empty");
        return INVALID_ARGUMENT;
    }
    if (!dirIs(dbpath)) {
        miniDBServerLog(LOG_ERROR,"Failed to init DB, path is not a directory: %s", dbpath);
        return INVALID_ARGUMENT;
    }
    miniDBServerLog(LOG_INFO, "dbInit");
    trxKit* trx_kit = trxKitCreate(trx_kit_name);
    if (trx_kit == NULL) {
        miniDBServerLog(LOG_ERROR, "Failed to create trx kit: %s", trx_kit_name);
        return INVALID_ARGUMENT;
    }
    // trxKitReset(trx_kit);

    d->buffer_pool_manager = bufferPoolManagerCreate();
    struct diskDoubleWriteBuffer* dblwr_buffer = diskDoubleWriteBufferCreate(d->buffer_pool_manager);
    const char *double_write_buffer_filename = "dblwr.db";
    sds double_write_buffer_file_path = sdscatfmt(sdsempty(), "%s/%s", dbpath, double_write_buffer_filename);
    int rc = diskDoubleWriteBufferOpenFile(dblwr_buffer, double_write_buffer_file_path);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to open double write buffer file. file=%s, rc=%d",
              double_write_buffer_file_path, rc);
        return rc;
    }
    rc = bufferPoolManagerInit(d->buffer_pool_manager, dblwr_buffer);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to init buffer pool manager. dbpath=%s, rc=%d", dbpath, rc);
        return rc;
    }    
    logHandler* tmp_log_handler = NULL;
    d->log_handler = logHandlerCreate(log_handler_name);
    if (d->log_handler == NULL) {
        miniDBServerLog(LOG_WARN, "Failed to create log handler: %s", log_handler_name);
        return INVALID_ARGUMENT;
    }
    sds clog_path = sdscatfmt(sdsempty(), "%s/clog", dbpath);
    rc = d->log_handler->init(d->log_handler, clog_path);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to init log handler. dbpath=%s, rc=%d", dbpath, rc);
        return rc;
    }
    d->name = sdsnew(dbname);
    d->path = dbpath;

    // 加载数据库本身的元数据
    rc = dbMetaInit(d);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to init meta. dbpath=%s, rc=%d", dbpath, (rc));
        return rc;
    }

    // 打开所有表
    // 在实际生产数据库中，直接打开所有表，可能耗时会比较长
    rc = db_open_all_tables(d);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to open all tables. dbpath=%s, rc=%s", dbpath, (rc));
        return rc;
    }

    rc = init_dblwr_buffer(d);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to init dblwr buffer. rc = %d", (rc));
        return rc;
    }

    // 尝试恢复数据库，重做redo日志
    rc = dbRecover(d);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to recover db. dbpath=%s, rc=%d", dbpath, (rc));
        return rc;
    }
    return SUCCESS;
}

void dbRelease(db* d) {
    zfree(d);
}