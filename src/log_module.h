#ifndef __LATTE_MINIDB_LOG_MODULE_H
#define __LATTE_MINIDB_LOG_MODULE_H

typedef enum logModuleId {
    BUFFER_POOL,     /// 缓冲池
    BPLUS_TREE,      /// B+树
    RECORD_MANAGER,  /// 记录管理
    TRANSACTION      /// 事务
} logModuleId;

typedef struct logModule {
    logModuleId id;
} logModule;

#endif