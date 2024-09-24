#include "record_log.h"

recordLogReplayer* recordLogReplayerCreate(buffer_pool_manager_t* manager) {
    recordLogReplayer* replayer = zmalloc(sizeof(recordLogReplayer));
    replayer->manager = manager;
    return replayer;
}