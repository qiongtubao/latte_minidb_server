#include "bplus_tree_log.h"

bplusTreeLogReplayer* bplusTreeLogReplayerCreate(bufferPoolManager* manager) {
    bplusTreeLogReplayer* replayer = zmalloc(sizeof(bplusTreeLogReplayer));
    replayer->manager = manager;
    return replayer;
}