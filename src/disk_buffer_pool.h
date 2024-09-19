#ifndef __LATTE_DISK_BUFFER_POOL_H
#define __LATTE_DISK_BUFFER_POOL_H




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