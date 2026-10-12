#ifndef SERVER_HPP
#define SERVER_HPP

#include "Tintin_reporter.hpp"
#include <vector>
#include <string>

#define PORT 4242
#define MAX_CLIENTS 3

class Server
{
public:
    Server(Tintin_reporter &reporter);
    ~Server();

    bool start();
    void run();

private:
    Server(const Server &);
    Server &operator=(const Server &);

    void acceptClient();
    bool readClient(size_t idx);
    void removeClient(size_t idx);

    int _listenFd;
    Tintin_reporter &_log;
    bool _running;
    std::vector<int> _clients;
    std::vector<std::string> _buffers;
};

#endif