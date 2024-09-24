#ifndef __LATTE_LOG_BUFFER_H
#define __LATTE_LOG_BUFFER_H

#include "mutex/mutex.h"

typedef struct logEntryBuffer {
    latte_mutex* mutex;
    
} logEntryBuffer;

logEntryBuffer* logEntryBufferCreate();

#endif