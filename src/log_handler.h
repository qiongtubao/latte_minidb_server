#ifndef __LATTE_LOG_HANDLER_H
#define __LATTE_LOG_HANDLER_H

#include "types.h"
#include "log_replayer.h"
typedef enum logHandlerType {
    DISK,
    VACUOUS
} logHandlerType;
/**
 * @brief 对外提供服务的CLog模块
 * @ingroup CLog
 * @details 该模块负责日志的写入、读取、回放等功能。
 * 会在后台开启一个线程，一直尝试刷新内存中的日志到磁盘。
 * 所有的CLog日志文件都存放在指定的目录下，每个日志文件按照日志条数来划分。
 */
typedef struct logHandler logHandler;
typedef struct logHandler {
    /**
     *  @brief 初始化日志模块
     *  @param path 日志文件存放的目录
     */
    int (*init)(logHandler* handler, const char* path);
    int (*start)(logHandler* handler);
    int (*stop)(logHandler* handler);
    int (*replay)(logHandler* handler, logReplayer* replayer, LSN start_lsn);
    int (*waitLsn)(logHandler* handler, LSN lsn);
} logHandler;
logHandler* logHandlerCreate(const char* name);

#endif

