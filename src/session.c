#include "session.h"
#include "default_handler.h"
#include "log/log.h"
struct session* default_session() {
    struct session* s = zmalloc(sizeof(session));
    return s;
}

void session_set_current_db(struct session* s, db* d) {
  if (d == NULL) {
    log_warn("latte_lib", "session set current db is null");
    return;
  }
  log_trace("latte_lib", "change db to %s", d->name);
  s->db = d;
}