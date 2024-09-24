#include "frame.h"
#include "zmalloc/zmalloc.h"
#include "log.h"
#include "utils.h"
#include "utils/atomic.h"
#include <time.h>
#include "utils/utils.h"

frame_id_t* frame_id_new(int buffer_pool_id, int32_t page_num) {
    frame_id_t* id = zmalloc(sizeof(frame_id_t));
    id->buffer_pool_id = buffer_pool_id;
    id->page_num = page_num;
    return id;
}

void frame_id_delete(frame_id_t* id) {
    zfree(id);
}

sds frame_id_to_sds(frame_id_t* frame_id) {
    return sds_cat_printf(sds_empty(), "buffer_pool_id: %d, page_num:%d", frame_id->buffer_pool_id, frame_id->page_num);
}


frame_t* frame_new() {
    frame_t* frame = zmalloc(sizeof(frame_t));
    return frame;
}

void frame_delete(frame_t* frame) {
    zfree(frame);
}

void frame_set_check_sum(frame_t* frame, CheckSum checksum) {
    frame->page->check_sum = checksum;
}

void frame_clear_dirty(frame_t* frame) {
    frame->dirty = false;
}
bool frame_can_purge(frame_t* frame) {
    int pin_count; 
    latte_atomic_get(frame->pin_count, pin_count);
    return pin_count == 0; 
}
//锁定frame   
void frame_pin(frame_t* frame) {
    latte_mutex_lock(frame->debug_lock);
    int pin_count = latte_atomic_incr(frame->pin_count, 1);
    
    //int xid = get_default_debug_xid(); 暂时不知道xid的用处  还要引入session  之后再理解
    int xid = 1;

    miniDBServerLog(LOG_TRACE , "after frame pin. "
          "this=%p, write locker=%p, read locker has xid ? pin=%d, frameId=%s, xid=%ld, lbt=trace\n",
          frame, frame->write_locker, xid == 1, pin_count, frame->frame_id, xid);
    latte_mutex_unlock(frame->debug_lock);

}
//解除锁定
int frame_unpin(frame_t* frame) {
    
    //int xid = get_default_debug_xid();
    int xid = 1;
    int pin_count;
    latte_atomic_get(frame->pin_count, pin_count);
    minidb_assert(pin_count > 0, "try to unpin a frame that pin count <= 0."
                         "this=%p, pin=%d, frameId=%s, xid=%ld, lbt=trace\n",
                         frame, pin_count, frame->frame_id, xid);
    latte_mutex_lock(frame->debug_lock);
    pin_count = latte_atomic_decr(frame->pin_count, 1);
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
    latte_mutex_unlock(frame->debug_lock);
    return frame->pin_count;
}

sds frame_to_sds(frame_t* frame) {
    sds frame_id_str = frame_id_to_sds(frame->frame_id);
    sds result = sds_cat_printf(sds_empty(), "frame id: %s, dirty=%d, pin=%d ,lsn=%d", frame_id_to_sds(frame->frame_id), frame->dirty, frame->pin_count, frame->page->lsn);
    sds_delete(frame_id_str);
    return result;
}


void frame_access(frame_t* frame) {
    frame->acc_time = current_monitonic_time();
}

void frame_set_page_num(frame_t* frame, PageNum page_num) {
    frame->frame_id->page_num = page_num;
}

void frame_set_buffer_pool_id(frame_t* frame, int buffer_pool_id) {
    frame->frame_id->buffer_pool_id = buffer_pool_id;
}