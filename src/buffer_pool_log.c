
#include "buffer_pool_log.h"

bufferPoolLogReplayer* bufferPoolLogReplayerCreate(buffer_pool_manager_t* manager) {
    bufferPoolLogReplayer* replayer = zmalloc(sizeof(bufferPoolLogReplayer));
    replayer->manager = manager;
    return replayer;
}