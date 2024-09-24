#ifndef __LATTE_RECORD_LOG_H
#define __LATTE_RECORD_LOG_H

#include "log_replayer.h"
#include "disk_buffer_pool.h"
typedef struct recordLogReplayer {
    logReplayer supper;
    buffer_pool_manager_t* manager;
} recordLogReplayer;
recordLogReplayer* recordLogReplayerCreate(buffer_pool_manager_t* manager);

#endif