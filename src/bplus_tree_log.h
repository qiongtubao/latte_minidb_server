#ifndef __LATTE_MINIDB_BPLUS_TREE_LOG_H
#define __LATTE_MINIDB_BPLUS_TREE_LOG_H

#include "disk_buffer_pool.h"
#include "log_replayer.h"

typedef struct bplusTreeLogReplayer {
    logReplayer supper;
    bufferPoolManager* manager;
} bplusTreeLogReplayer;
bplusTreeLogReplayer* bplusTreeLogReplayerCreate(bufferPoolManager* manager);

#endif
