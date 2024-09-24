
#ifndef __LATTE_BUFFER_POOL_LOG_H
#define __LATTE_BUFFER_POOL_LOG_H

#include "log_replayer.h"
#include "disk_buffer_pool.h"

typedef struct bufferPoolLogReplayer {
    logReplayer supper;
    buffer_pool_manager_t* manager;
} bufferPoolLogReplayer;

bufferPoolLogReplayer* bufferPoolLogReplayerCreate(buffer_pool_manager_t* manager);



#endif