#include "Server.hpp"
#include "Daemon.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>

Server::Server(Tintin_reporter &reporter)
    : _listenFd(-1), _log(reporter), _running(true) {}

Server::~Server()
{
    for (size_t i = 0; i < _clients.size(); i++)
        close(_clients[i]);
    if (_listenFd >= 0)
        close(_listenFd);
}

bool Server::start()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        return false;

    int yes = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(_listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return false;
    if (listen(_listenFd, 5) < 0)
        return false;
    return true;
}

void Server::acceptClient()
{
    int client = accept(_listenFd, NULL, NULL);
    if (client < 0)
        return;
    if (_clients.size() >= MAX_CLIENTS)
    {
        _log.log("INFO", "Connection refused: too many clients.");
        close(client);
        return;
    }
    _clients.push_back(client);
    _buffers.push_back("");
    _log.log("INFO", "Client connected.");
}

void Server::removeClient(size_t idx)
{
    close(_clients[idx]);
    _clients.erase(_clients.begin() + idx);
    _buffers.erase(_buffers.begin() + idx);
    _log.log("INFO", "Client disconnected.");
}

bool Server::readClient(size_t idx)
{
    char tmp[1024];
    ssize_t n = recv(_clients[idx], tmp, sizeof(tmp), 0);
    if (n <= 0)
        return false;                      // client left or error

    std::string &buffer = _buffers[idx];
    buffer.append(tmp, n);

    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line == "quit")
        {
            _log.log("INFO", "Request: Quit.");
            _running = false;
        }
        else
            _log.log("LOG", "User input: " + line);
    }
    return true;
}

void Server::run()
{
    while (_running)
    {
        // 1. Was a signal received?
        int sig = Daemon::pendingSignal();
        if (sig)
        {
            std::ostringstream oss;
            oss << "Signal handler: received signal " << sig
                << " (" << strsignal(sig) << ").";
            _log.log("INFO", oss.str());
            break;
        }

        // 2. Build the list to watch: front door + every client
        std::vector<struct pollfd> fds;
        struct pollfd p;
        p.events = POLLIN;
        p.revents = 0;

        p.fd = _listenFd;
        fds.push_back(p);
        for (size_t i = 0; i < _clients.size(); i++)
        {
            p.fd = _clients[i];
            fds.push_back(p);
        }

        // 3. Sleep until something happens (max 0.5 s, so signals are noticed)
        int ret = poll(&fds[0], fds.size(), 500);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ret == 0)
            continue;

        // 4. Clients first, from last to first, so removing one doesn't shift the others
        for (size_t i = _clients.size(); i > 0; i--)
        {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
            {
                if (!readClient(i - 1))
                    removeClient(i - 1);
            }
        }

        // 5. New connection last, because it changes _clients
        if (fds[0].revents & POLLIN)
            acceptClient();
    }
}