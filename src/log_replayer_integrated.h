

#ifndef __LATTE_MINIDB_LOG_REPLAYER_INTEGRATED_H
#define __LATTE_MINIDB_LOG_REPLAYER_INTEGRATED_H

#include "log_replayer.h"
#include "bplus_tree_log.h"
#include "buffer_pool_log.h"
#include "record_log.h"

typedef struct integratedLogReplayer {
    logReplayer supper;
    bufferPoolLogReplayer*  buffer_pool_log_replayer;  ///< 缓冲池日志回放器
    recordLogReplayer*      record_log_replayer;       ///< record manager 日志回放器
    bplusTreeLogReplayer*   bplus_tree_log_replayer;   ///< bplus tree 日志回放器
    logReplayer*            trx_log_replayer;          ///< trx 日志回放器  unique_ptr(智能指针)
} integratedLogReplayer;

/**
   * @brief 构造函数
   * @details
   * 区别于另一个构造函数，这个构造函数可以指定不同的事务日志回放器。比如进程启动时可以指定选择使用VacuousTrx还是MvccTrx。
   */
integratedLogReplayer* integratedLogReplayerCreate(bufferPoolManager* manager,logReplayer* logReplayer);

#endif