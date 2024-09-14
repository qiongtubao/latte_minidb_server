#include "server.h"
#include <string.h>
#include "utils.h"

sds* parseArgv(int argc, char** argv, int* len) {
    char** result = zmalloc(sizeof(sds)*(argc + 1));
    result[argc] = NULL;
    for(int j = 0; j < argc; j++) {
        result[j] = sdsnewlen(argv[j], strlen(argv[j]));
    }
    *len = argc;
    return result;
}



//初始化dbserver
int initMiniDBServer(latteMiniDBServer* server, int argc, char** argv) {
    int len = 0;
    server->exec_argv = parseArgv(argc, argv, &len);
    server->exec_argc = len;

    server->executable = getAbsolutePath(argv[0]);
    server->config = createServerConfig();

    startLatteServer(&server->server);
    return 1;
}

int startMiniDBServer(struct latteMiniDBServer* redisServer) {
    return 1;
}