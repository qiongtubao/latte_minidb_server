#include "bplus_tree_log.h"

bplusTreeLogReplayer* bplusTreeLogReplayerCreate(buffer_pool_manager_t* manager) {
    bplusTreeLogReplayer* replayer = zmalloc(sizeof(bplusTreeLogReplayer));
    replayer->manager = manager;
    return replayer;
}