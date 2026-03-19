#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>

static constexpr int    PORT          = 8080;
static constexpr int    BUFFER_SIZE   = 1024;
static constexpr char   SERVER_IP[]   = "127.0.0.1";
static constexpr char   MESSAGE[]     = "Hello! <3";
static constexpr int    ROUNDS        = 5;

int main() {
    for (int i = 0; i < ROUNDS; ++i) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            return 1;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family      = AF_INET;
        server_addr.sin_port        = htons(PORT);

        if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
            perror("inet_pton");
            close(sock);
            return 1;
        }

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            perror("connect");
            close(sock);
            return 1;
        }

        std::cout << "[round " << i + 1 << "] Sending:  " << MESSAGE << std::endl;
        if (send(sock, MESSAGE, strlen(MESSAGE), 0) < 0) {
            perror("send");
            close(sock);
            return 1;
        }

        char buffer[BUFFER_SIZE] = {0};
        int bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes > 0) {
            std::cout << "[round " << i + 1 << "] Received: " << buffer << std::endl;
        } else if (bytes < 0) {
            perror("recv");
        }

        close(sock);
    }

    return 0;
}
