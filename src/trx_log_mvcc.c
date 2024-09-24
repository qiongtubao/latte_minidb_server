
#include "trx_log_mvcc.h"
#include "code.h"
#include <assert.h>
#include "log_module.h"
#include "log.h"

int mvcc_trx_log_replay(logReplayer *logReplayer, const logEntry *entry) {
    mvccTrxLogReplayer* replayer = (mvccTrxLogReplayer*)logReplayer;
    int rc = SUCCESS;
    // 由于当前没有check point，所以所有的事务都重做。
    assert(entry->header.module_id == TRANSACTION);
    // assert(entry->header.module_id == TRANSACTION, "invalid log module id: %d", entry->header.module_id);
    if (entry->header.size < MvccTrxLogHeader_SIZE) {
        miniDBServerLog(LOG_WARN, "invalid log entry size: %d, trx log header size:%ld", entry->header.size, MvccTrxLogHeader_SIZE);
        return LOG_ENTRY_INVALID;
    }
    mvccTrxLogHeader* header = (mvccTrxLogHeader*)entry->data;
    mvccTrx* trx = NULL;
    dictEntry* node = dictFind(replayer->trx_map, header->trx_id);
    int created = 0;
    if (node == NULL) {
        trx = replayer->trx_kit->supper.create_trx(replayer->trx_kit, replayer->log_handler, header->trx_id);
        created = 1;
    } else {
        trx = dictGetVal(node);
    }
    /// 直接调用事务代码自己的重放函数
    rc = trx->supper.redo(trx, replayer->d, entry);
    /// 如果事务结束了，需要从内存中把它删除
    if (header->operation_type == ROLLBACK ||
        header->operation_type == COMMIT) {
        if (created == 0) { 
            replayer->trx_kit->supper.destroy_trx(replayer->trx_kit, trx);
            dictDelete(replayer->trx_map, header->trx_id);
        }
    }
    return rc;
}

logReplayer mvccLogReplayerType = {
    .replay = mvcc_trx_log_replay
};

mvccTrxLogReplayer* mvccTrxLogReplayerCreate(db* d, mvccTrxKit* kit, logHandler* log_handler) {
    mvccTrxLogReplayer* replayer = zmalloc(sizeof(mvccTrxLogReplayer));
    replayer->supper = mvccLogReplayerType;
    replayer->d = d;
    replayer->trx_kit = kit;
    replayer->log_handler = log_handler;
    return replayer;
}