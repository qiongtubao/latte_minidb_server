#ifndef __LATTE_TRX_MVCC_H
#define __LATTE_TRX_MVCC_H

#include "trx.h"
#include "utils/atomic.h"
#include "mutex/mutex.h"
#include "list/list.h"
typedef struct mvccTrxKit {
    trxKit basic;
    struct list* fields;
    latteAtomic int current_trx_id;
    latte_mutex* lock;
    struct list* trxes;
} mvccTrxKit;

mvccTrxKit* mvccTrxKitCreate();
int mvccTrxKitInit(mvccTrxKit* trx);
void mvccTrxKitRelease(mvccTrxKit* trx); 
#endif