#ifndef NETWORK_H
#define NETWORK_H

class Network{
    public:
        Network(int port);
        ~Network();

        bool start();
        void run();

    private:
        int server_fd;
        int port;
};

#endif