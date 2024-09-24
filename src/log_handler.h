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
typedef struct log_handler_t log_handler_t;
typedef struct log_handler_t {
    /**
     *  @brief 初始化日志模块
     *  @param path 日志文件存放的目录
     */
    int (*init)(log_handler_t* handler, const char* path);
    int (*start)(log_handler_t* handler);
    int (*stop)(log_handler_t* handler);
    int (*replay)(log_handler_t* handler, logReplayer* replayer, LSN start_lsn);
    int (*waitLsn)(log_handler_t* handler, LSN lsn);
} log_handler_t;
log_handler_t* log_handler_new(const char* name);

#endif

