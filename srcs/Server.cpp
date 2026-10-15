#include "Server.hpp"
#include "Daemon.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>

#define MAX_LINE 4096

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
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        _log.log("ERROR", "setsockopt(SO_REUSEADDR) failed.");

    // Non-blocking so accept() can never freeze the loop.
    int flags = fcntl(_listenFd, F_GETFL, 0);
    if (flags < 0 || fcntl(_listenFd, F_SETFL, flags | O_NONBLOCK) < 0)
        return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(_listenFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
        return false;
    if (listen(_listenFd, 5) < 0)
        return false;
    return true;
}

void Server::acceptClient()
{
    int client = accept(_listenFd, NULL, NULL);
    if (client < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR
            && errno != ECONNABORTED)
            _log.log("ERROR", std::string("accept failed: ") + strerror(errno));
        return;
    }
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

void Server::handleLine(std::string line)
{
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    if (line.empty())
        return;
    if (line == "quit")
    {
        _log.log("INFO", "Request quit.");
        _running = false;
    }
    else
        _log.log("LOG", "User input: " + line);
}

void Server::removeClient(size_t idx)
{
    if (_running && !_buffers[idx].empty())
        handleLine(_buffers[idx]);
    close(_clients[idx]);
    _clients.erase(_clients.begin() + idx);
    _buffers.erase(_buffers.begin() + idx);
    _log.log("INFO", "Client disconnected.");
}

bool Server::readClient(size_t idx)
{
    char    tmp[1024];
    ssize_t n = recv(_clients[idx], tmp, sizeof(tmp), 0);

    if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
        return true;
    if (n <= 0)
        return false;                       // client left or error

    std::string &buffer = _buffers[idx];
    buffer.append(tmp, static_cast<size_t>(n));

    size_t pos;
    while (_running && (pos = buffer.find('\n')) != std::string::npos)
    {
        std::string line = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        handleLine(line);
    }
    if (_running && buffer.size() > MAX_LINE)   // no newline in sight: flush
    {
        handleLine(buffer);
        buffer.clear();
    }
    return true;
}

void Server::run()
{
    while (_running)
    {
        int sig = Daemon::pendingSignal();
        if (sig)
        {
            std::ostringstream oss;
            oss << "Signal handler: received signal " << sig
                << " (" << strsignal(sig) << ").";
            _log.log("INFO", oss.str());
            break;
        }

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

        int ret = poll(&fds[0], fds.size(), 500);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            _log.log("ERROR", std::string("poll failed: ") + strerror(errno));
            break;
        }
        if (ret == 0)
            continue;

        for (size_t i = fds.size() - 1; i > 0; i--)
        {
            if (fds[i].revents & (POLLIN | POLLHUP | POLLERR))
            {
                if (!readClient(i - 1))
                    removeClient(i - 1);
            }
        }

        if (_running && (fds[0].revents & POLLIN))
            acceptClient();
    }
}