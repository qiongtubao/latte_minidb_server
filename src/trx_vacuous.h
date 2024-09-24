#ifndef __LATTE_TRX_VACUOUS_H
#define __LATTE_TRX_VACUOUS_H

#include "trx.h"
typedef struct vacuousTrxKit {
    trxKit supper;
} vacuousTrxKit;

vacuousTrxKit* vacuousTrxKitCreate();
int vacuousTrxKitInit(vacuousTrxKit* trx);
void vacuousTrxKitRelease(vacuousTrxKit* trx); 
#endif
