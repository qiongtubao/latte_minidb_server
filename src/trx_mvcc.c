#include "trx_mvcc.h"
#include <stdbool.h>
#include "code.h"
#include "attr_type.h"
#include "log/log.h"
#include "field_meta.h"
#include "db.h"
#include "trx_log_mvcc.h"
logReplayer* mvcc_create_log_replayer(trxKit *kit, db* d, logHandler* handler) {
    return mvccTrxLogReplayerCreate(d, kit, handler);
}
trxKit mvccTrxKitType = {
    .create_log_replayer = mvcc_create_log_replayer
};

mvccTrxKit* mvccTrxKitCreate() {
    mvccTrxKit* trx = zmalloc(sizeof(mvccTrxKit));
    trx->supper = mvccTrxKitType;
    trx->current_trx_id = 0;
    trx->lock = mutexCreate();
    trx->fields = listCreate();
    trx->trxes = listCreate();
    return trx;
}

int mvccTrxKitInit(mvccTrxKit* trx) {
    listAddNodeTail(trx->fields, 
        fieldMetaCreate("__trx_xid_begin", INTS, 0 /*attr_offset*/, 4 /*attr_len*/, false /*visible*/, -1/*field_id*/));
    listAddNodeTail(trx->fields, 
            fieldMetaCreate("__trx_xid_end", INTS, 0 /*attr_offset*/, 4 /*attr_len*/, false /*visible*/, -2/*field_id*/));
    log_info("latte_lib","init mvcc trx kit done.");
    return SUCCESS;
}

void mvccTrxKitRelease(mvccTrxKit* trx) {
    mutexRelease(trx->lock);
    listRelease(trx->fields);
    listRelease(trx->trxes);
    zfree(trx);
}