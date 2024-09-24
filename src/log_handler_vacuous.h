#ifndef __LATTE_LOG_HANDLER_VACUOUS_H
#define __LATTE_LOG_HANDLER_VACUOUS_H

#include "log_handler.h"
typedef struct vacuousLogHandler {
    logHandler supper;
} vacuousLogHandler;

vacuousLogHandler* vacuousLogHandlerCreate();

#endif