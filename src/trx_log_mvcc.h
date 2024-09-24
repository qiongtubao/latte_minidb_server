#ifndef __LATTE_TRX_LOG_MVCC_H
#define __LATTE_TRX_LOG_MVCC_H

#include "log_replayer.h"
#include "db.h"
#include "trx_mvcc.h"
#include "log_handler.h"
#include <stdlib.h>

typedef enum mvccTrxLogOperation {
    INSERT_RECORD,  ///< 插入一条记录
    DELETE_RECORD,  ///< 删除一条记录
    COMMIT,         ///< 提交事务
    ROLLBACK        ///< 回滚事务
} mvccTrxLogOperation;

typedef struct mvccTrxLogReplayer {
    logReplayer supper;
    db* d;                      ///< 所属数据库
    struct mvccTrxKit* trx_kit;         ///< 事务管理器
    logHandler* log_handler;    ///< 日志处理器
    ///< 事务ID到事务的映射。在重做结束后，如果还有未提交的事务，需要回滚。
    dict* trx_map;      //map<int32_t, mvccTrx*>
} mvccTrxLogReplayer;

typedef struct mvccTrxLogHeader {
    mvccTrxLogOperation operation_type;  ///< 操作类型
    int32_t trx_id;          ///< 事务ID
} mvccTrxLogHeader;

static const int32_t MvccTrxLogHeader_SIZE = sizeof(mvccTrxLogHeader);

mvccTrxLogReplayer* mvccTrxLogReplayerCreate(db* d,struct mvccTrxKit* kit, logHandler* log_handler);

#endif