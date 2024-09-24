#ifndef __LATTE_MINIDB_LOG_REPLAYER_H
#define __LATTE_MINIDB_LOG_REPLAYER_H


#include "log_entry.h"

typedef struct logReplayer logReplayer;
/**
 * @brief 日志回放接口类
 * @ingroup CLog
 */
typedef struct logReplayer {
    int (*replay)(logReplayer* replayer,const logEntry* entry);
    int (*on_done)(logReplayer* replayer);
} logReplayer;

#endif