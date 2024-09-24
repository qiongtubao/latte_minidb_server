#ifndef __LATTE_TRX_H
#define __LATTE_TRX_H
#include "sds/sds.h"
#include "log_handler.h"
#include "log_replayer.h"
#include "log_entry.h"

typedef struct db db;
/**
   * @brief 操作的类型
   * @ingroup Transaction
   */
typedef enum trxOperationType {
    INSERT,
    UPDATE,
    DELETE,
    TRX_UNDEFINED,
} trxOperationType;

/**
 * @brief 描述一个操作，比如插入、删除行等
 * @ingroup Transaction
 * @details 通常包含一个操作的类型，以及操作的对象和具体的数据
 * @note 这个名称太通用，可以考虑改成更具体的名称
 */
typedef struct Operation {
    trxOperationType type;
    struct table* tab;
    PageNum page_num;
    SlotNum slot_num;
} Operation;

/**
 * @brief 事务管理器的类型
 * @ingroup Transaction
 * @details 进程启动时根据事务管理器的类型来创建具体的对象
 */
typedef enum trxKitType {
    TRXKIT_VACUOUS,    ///< 空的事务管理器，不做任何事情
    MVCC,       ///< 支持MVCC的事务管理器
} trxKitType;

typedef struct Trx Trx;
/**
 * @brief 事务接口
 * @ingroup Transaction
 */
typedef struct Trx { 
    int (*redo)(Trx* trx, db* d, logEntry* entry);
} Trx;

typedef struct trxKit trxKit;
/**
 * @brief 事务管理器
 * @ingroup Transaction
 */
typedef struct trxKit {
    Trx* (*create_trx)(trxKit* kit, logHandler* hanadler, int32_t trx_id);
    logReplayer* (*create_log_replayer)(trxKit* kit, db* d, logHandler* handler);
    void (*destroy_trx)(trxKit* kit, Trx* trx);
} trxKit;


trxKit* trxKitCreate(sds name);

#endif