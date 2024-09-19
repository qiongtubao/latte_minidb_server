#ifndef __LATTE_TRX_H
#define __LATTE_TRX_H
#include "sds/sds.h"
#include "log_handler.h"
/**
 * @brief 事务管理器的类型
 * @ingroup Transaction
 * @details 进程启动时根据事务管理器的类型来创建具体的对象
 */
typedef enum trxKitType {
    VACUOUS,    ///< 空的事务管理器，不做任何事情
    MVCC,       ///< 支持MVCC的事务管理器
} trxKitType;

typedef struct trxKit {
    trxKitType type;
} trxKit;

trxKit* trxKitCreate(sds name);

#endif