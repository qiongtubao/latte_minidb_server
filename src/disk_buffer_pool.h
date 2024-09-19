#ifndef __LATTE_DISK_BUFFER_POOL_H
#define __LATTE_DISK_BUFFER_POOL_H

#include "mutex/mutex.h"


typedef struct bpFrameManager {

} bpFrameManager;

typedef struct doubleWriteBuffer {

} doubleWriteBuffer;

typedef struct bufferPoolLogHandler {

} bufferPoolLogHandler;

typedef struct bufferPoolManager {
    struct bpFrameManager* frame_manager;
    struct doubleWriteBuffer* dblwr_buffer;
    mutex lock;

    
} bufferPoolManager;

#endif