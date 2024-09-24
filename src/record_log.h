#ifndef __LATTE_RECORD_LOG_H
#define __LATTE_RECORD_LOG_H

#include "log_replayer.h"
#include "disk_buffer_pool.h"
typedef struct recordLogReplayer {
    logReplayer supper;
    bufferPoolManager* manager;
} recordLogReplayer;
recordLogReplayer* recordLogReplayerCreate(bufferPoolManager* manager);

#endif