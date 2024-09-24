#ifndef __LATTE_LOG_HANDLER_VACUOUS_H
#define __LATTE_LOG_HANDLER_VACUOUS_H

#include "log_handler.h"
typedef struct vacuous_log_handler_t {
    log_handler_t supper;
} vacuous_log_handler_t;

vacuous_log_handler_t* vacuous_log_handler_new();

#endif