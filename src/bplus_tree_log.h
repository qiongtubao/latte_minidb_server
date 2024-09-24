#ifndef __LATTE_MINIDB_BPLUS_TREE_LOG_H
#define __LATTE_MINIDB_BPLUS_TREE_LOG_H

#include "disk_buffer_pool.h"
#include "log_replayer.h"

typedef struct bplusTreeLogReplayer {
    logReplayer supper;
    buffer_pool_manager_t* manager;
} bplusTreeLogReplayer;
bplusTreeLogReplayer* bplusTreeLogReplayerCreate(buffer_pool_manager_t* manager);

#endif
