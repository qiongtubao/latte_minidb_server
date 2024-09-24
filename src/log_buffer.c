#include "log_buffer.h"
#include "zmalloc/zmalloc.h"

logEntryBuffer* logEntryBufferCreate() {
    logEntryBuffer* buffer = zmalloc(sizeof(logEntryBuffer));
    return buffer;
}