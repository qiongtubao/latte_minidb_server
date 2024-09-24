
#include "log_replayer_integrated.h"


integratedLogReplayer* integratedLogReplayerCreate(bufferPoolManager* manager,logReplayer* log_replayer) {
    integratedLogReplayer* replayer = zmalloc(sizeof(integratedLogReplayer));
    replayer->buffer_pool_log_replayer = bufferPoolLogReplayerCreate(manager);
    replayer->record_log_replayer = recordLogReplayerCreate(manager);
    replayer->bplus_tree_log_replayer = bplusTreeLogReplayerCreate(manager);
    replayer->trx_log_replayer = log_replayer;
    return replayer;
}