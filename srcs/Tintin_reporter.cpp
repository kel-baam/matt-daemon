#include "Tintin_reporter.hpp"
#include <sys/stat.h>
#include <ctime>
#include <stdexcept>

Tintin_reporter::Tintin_reporter()
{
    mkdir("/var/log/matt_daemon", 0755);
    _file.open("/var/log/matt_daemon/matt_daemon.log", std::ios::app);
    if (!_file.is_open())
        throw std::runtime_error("Can't open /var/log/matt_daemon/matt_daemon.log");
}

Tintin_reporter::~Tintin_reporter()
{
    if (_file.is_open())
        _file.close();
}

std::string Tintin_reporter::_timestamp() const
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%d/%m/%Y-%H:%M:%S", t);
    return std::string(buf);
}

void Tintin_reporter::log(const std::string &level, const std::string &message)
{
    _file << "[" << _timestamp() << "] [ " << level << " ] - Matt_daemon: "
          << message << std::endl;   // endl also flushes, so nothing is lost
}