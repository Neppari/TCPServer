#include <iostream>
#include "network.h"

int main(int argc, char *argv[]) {
    int port = 8080;
    
    Network server(port);

    if (!server.start()) {
        return 1;
    }

    server.run();

    return 0;
}