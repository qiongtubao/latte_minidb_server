#include "record_manager.h"


recordFileHandler* recordFileHandlerCreate() {
    recordFileHandler* handler = zmalloc(sizeof(recordFileHandler));
    return handler;
}

