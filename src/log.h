#ifndef __LATTE_MINIDB_LOG_H
#define __LATTE_MINIDB_LOG_H
#include "log/log.h"

#define LATTE_MINIDB_SERVER_LOG_TAG "latte_mini_server"
#define miniDBServerLog(log, ...) log_log(LATTE_MINIDB_SERVER_LOG_TAG, log, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__) 


#endif