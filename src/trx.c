#include "trx.h"
#include "utils.h"
#include <string.h>
#include <strings.h>
#include "log/log.h"
#include "code.h"
#include "trx_vacuous.h"
#include "trx_mvcc.h"

trxKit* trxKitCreate(sds name) {
    trxKit * trx_kit = NULL;
    int rc;
    if (is_blank(name) || 
        0 == strcasecmp(name, "vacuous")) {
        trx_kit = vacuousTrxKitCreate();
        rc = vacuousTrxKitInit(trx_kit);
        if (isRcFail(rc)) {
            log_error("failed to init trx kit. name=%s, rc=%d", name, rc);
            vacuousTrxKitRelease(trx_kit);
            return NULL;
        }
    } else if (0 == strcasecmp(name, "mvcc")) {
        trx_kit = mvccTrxKitCreate();
        rc = mvccTrxKitInit(trx_kit);
        if (isRcFail(rc)) {
            log_error("failed to init trx kit. name=%s, rc=%d", name, rc);
            mvccTrxKitRelease(trx_kit);
            return NULL;
        }
    } else {
        log_error("latte_lib", "unknown trx kit name, name=%s", name);
        return NULL;
    }

}
