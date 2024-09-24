
#include "buffer_pool_log.h"

bufferPoolLogReplayer* bufferPoolLogReplayerCreate(bufferPoolManager* manager) {
    bufferPoolLogReplayer* replayer = zmalloc(sizeof(bufferPoolLogReplayer));
    replayer->manager = manager;
    return replayer;
}