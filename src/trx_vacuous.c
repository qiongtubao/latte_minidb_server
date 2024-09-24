#include "trx_vacuous.h"
#include "code.h"
vacuousTrxKit* vacuousTrxKitCreate() {
    vacuousTrxKit* vacuous = zmalloc(sizeof(vacuousTrxKit));
    
    return vacuous;
}

int vacuousTrxKitInit(vacuousTrxKit* trx) {
    return SUCCESS;
}

void vacuousTrxKitRelease(vacuousTrxKit* trx) {
    zfree(trx);
}