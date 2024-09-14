#include "main.h"
#include "sds/sds.h"
#include <stdio.h>


struct latteMiniDBServer miniDbServer;

int main(int argc, char** argv) {
    if(!initMiniDBServer(&miniDbServer, argc, argv)) {
        printf("init miniDB fail");
    } else if (!startMiniDBServer(&miniDbServer)) {
        printf("start miniDB fail");
    }
}