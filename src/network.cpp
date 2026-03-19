//server socket, port, listen for incoming connections
// pass incoming data to protocol
#include "network.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <cstring>
#include <memory>

Network::Network(int port) : port(port), server_fd(-1) {}

Network::~Network() {
    if (server_fd != -1) {
        close(server_fd);
    }
}

bool Network::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        return false;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return false;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return false;
    }

    return true;
}

void Network::run() {
    sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    while(true) {
        std::cout << "Waiting for connection" << std::endl;

        int new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        std::cout << "Client connected" << std::endl;

        char buffer[1024] = {0};
        int valread = read(new_socket, buffer, 1024);

        if (valread > 0) {
            std::cout << "Received: " << buffer << std::endl;

            const char* response = "Hello from server\n";
            send(new_socket, response, strlen(response), 0);
        }

        close(new_socket);
        std::cout << "Client disconnected" << std::endl;
    }
}

/*typedef int SOCKET;
#define INVALID_SOCKET -1;
#define SOCKET_ERROR -1;

class Network {
    private:
        static constexpr int PORT = 8080;
        static constexpr int BUFFER_SIZE = 1024;
        //PF_INET = IPv4, SOCK_STREAM = TCP
        SOCKET server_socket = socket(PF_INET, SOCK_STREAM, 0);
    public:
        Network() {
            //constructor
        }
};*/
