#ifndef __LATTE_LOG_BUFFER_H
#define __LATTE_LOG_BUFFER_H

#include "mutex/mutex.h"

typedef struct logEntryBuffer {
    latte_mutex_t* mutex;
    
} logEntryBuffer;

logEntryBuffer* logEntryBufferCreate();

#endif