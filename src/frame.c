#include "frame.h"
#include "zmalloc/zmalloc.h"
#include "log.h"
#include "utils.h"
#include "utils/atomic.h"

frameId* frameIdCreate(int buffer_pool_id, int32_t page_num) {
    frameId* id = zmalloc(sizeof(frameId));
    id->buffer_pool_id = buffer_pool_id;
    id->page_num = page_num;
    return id;
}

void frameIdRelease(frameId* id) {
    zfree(id);
}

Frame* frameCreate() {
    Frame* frame = zmalloc(sizeof(Frame));
    return frame;
}

void frameRelease(Frame* frame) {
    zfree(frame);
}

void frameSetCheckSum(Frame* frame, CheckSum checksum) {
    frame->page->check_sum = checksum;
}

void frameClearDirty(Frame* frame) {
    frame->dirty = false;
}

//锁定frame   
void frame_pin(Frame* frame) {
    mutexLock(frame->debug_lock);
    int pin_count = atomicIncr(frame->pin_count, 1);
    
    //int xid = get_default_debug_xid(); 暂时不知道xid的用处  还要引入session  之后再理解
    int xid = 1;

    miniDBServerLog(LOG_TRACE , "after frame pin. "
          "this=%p, write locker=%p, read locker has xid ? pin=%d, frameId=%s, xid=%ld, lbt=trace\n",
          frame, frame->write_locker, xid == 1, pin_count, frame->frame_id, xid);
    mutexUnlock(frame->debug_lock);

}
//解除锁定
int frame_unpin(Frame* frame) {
    
    //int xid = get_default_debug_xid();
    int xid = 1;
    int pin_count;
    atomicGet(frame->pin_count, pin_count);
    minidb_assert(pin_count > 0, "try to unpin a frame that pin count <= 0."
                         "this=%p, pin=%d, frameId=%s, xid=%ld, lbt=trace\n",
                         frame, pin_count, frame->frame_id, xid);
    mutexLock(frame->debug_lock);
    pin_count = atomicDecr(frame->pin_count, 1);
    miniDBServerLog(LOG_TRACE, "after frame unpin. "
                 "this=%p, write locker=%p, read locker has xid? %d, pin=%d, frameId=%s, xid=%ld, lbt=trace\n",
          frame, frame->write_locker, xid == 1, pin_count, frame->frame_id, xid);

    if (pin_count == 0) {
        minidb_assert(frame->write_locker == NULL,
               "frame unpin to 0 failed while someone hold the write lock. write locker=%p, frameId=%s, xid=%ld\n",
               frame->write_locker, frame->frame_id, xid);
        minidb_assert(frame->read_lockers == NULL,
               "frame unpin to 0 failed while someone hold the read locks. reader num=%d, frameId=%s, xid=%ld\n",
               frame->read_lockers == NULL ? 0 : 1, frame->frame_id, xid);
    }
    mutexUnlock(frame->debug_lock);
    return frame->pin_count;
}
