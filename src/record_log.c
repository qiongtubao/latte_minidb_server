#include "record_log.h"

recordLogReplayer* recordLogReplayerCreate(bufferPoolManager* manager) {
    recordLogReplayer* replayer = zmalloc(sizeof(recordLogReplayer));
    replayer->manager = manager;
    return replayer;
}