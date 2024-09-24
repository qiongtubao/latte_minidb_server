#ifndef __LATTE_TRX_MVCC_H
#define __LATTE_TRX_MVCC_H

#include "trx.h"
#include "utils/atomic.h"
#include "mutex/mutex.h"
#include "list/list.h"
typedef struct mvccTrxKit {
    trxKit supper;
    struct list_t* fields;
    latteAtomic int current_trx_id;
    latte_mutex_t* lock;
    struct list_t* trxes;
} mvccTrxKit;

typedef struct mvccTrx {
    Trx supper;
    mvccTrxKit* trx_kit;
    struct mvccTrxLogHeader* header;
    int32_t trx_id;
    bool started;
    bool recovering;
    list_t* operations; //vector<Operations>
} mvccTrx;

mvccTrxKit* mvccTrxKitCreate();
int mvccTrxKitInit(mvccTrxKit* trx);
void mvccTrxKitRelease(mvccTrxKit* trx); 
#endif