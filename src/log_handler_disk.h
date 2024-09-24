#ifndef __LATTE_LOG_HANDLER_DISK_H
#define __LATTE_LOG_HANDLER_DISK_H

#include "log_handler.h"
#include <pthread.h>
#include "utils/atomic.h"
#include <stdbool.h>
#include "sds/sds.h"
#include "log_file.h"
#include "log_buffer.h"


/**
 * @brief 对外提供服务的CLog模块
 * @ingroup CLog
 * @details 该模块负责日志的写入、读取、回放等功能。
 * 会在后台开启一个线程，一直尝试刷新内存中的日志到磁盘。
 * 所有的CLog日志文件都存放在指定的目录下，每个日志文件按照日志条数来划分。
 * 调用的顺序应该是：
 * @code {.cpp}
 * DiskLogHandler handler;
 * handler.init("/path/to/log");
 * // replay 在一次启动只运行一次，它会计算当前日志的一些最新状态，必须在start之前执行
 * handler.replay(replayer, start_lsn);
 * handler.start();
 * handler.stop();
 * handler.wait();
 * @endcode
 *
 */
typedef struct diskLogHandler {
    logHandler supper;
    pthread_t th;                   //刷新日志的线程
    latteAtomic bool running;       //是否还要继续运行
    logFileManager* file_manager;   //管理所有日志的文件
    logEntryBuffer* entry_buffer;   //缓存日志
    sds path;                       //日志文件存放的文件目录
} diskLogHandler;

diskLogHandler* diskLogHandlerCreate();

#endif