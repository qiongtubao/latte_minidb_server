#include "session.h"
#include "default_handler.h"
#include "log/log.h"
struct session* default_session() {
    struct session* s = zmalloc(sizeof(session));
    return s;
}


dbHandler* global_context;

void session_set_current_db(struct session* s, char* dbname) {
  struct Db  *db      = find_db(global_context, dbname);
  if (db == NULL) {
    log_warn("no such database: %s", dbname);
    return;
  }
  log_trace("latte_lib", "change db to %s", dbname);
  s->db = db;
}