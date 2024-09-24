#ifndef __LATTE_MINIDB_SERVER_H
#define __LATTE_MINIDB_SERVER_H

#include "server/server.h"
#include "default_handler.h"
config_manager_t* createServerConfig();




typedef struct latteMiniDBServer {
    struct latteServer server;
    int exec_argc;
    sds* exec_argv;
    sds executable; /** execut file path **/
    sds configfile;
    struct config* config;
    struct db_handler_t* hander;
} latteMiniDBServer;


#define DB_ERR 0
#define DB_OK 1

#define PRIVATE 
PRIVATE int initMiniDBServer(struct latteMiniDBServer* redisServer, int argc, sds* argv);
PRIVATE int startMiniDBServer(struct latteMiniDBServer* redisServer);



#endif /* __LATTE_MINIDB_SERVER_H */