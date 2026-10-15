#include "Tintin_reporter.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <ctime>
#include <stdexcept>

Tintin_reporter::Tintin_reporter()
{
    if (mkdir(LOG_DIR, 0755) == -1 && errno != EEXIST)
        throw std::runtime_error("Can't create " LOG_DIR);
    _file.open(LOG_FILE, std::ios::app);
    if (!_file.is_open())
        throw std::runtime_error("Can't open " LOG_FILE);
}

Tintin_reporter::~Tintin_reporter()
{
    if (_file.is_open())
        _file.close();
}

std::string Tintin_reporter::_timestamp() const
{
    time_t      now = time(NULL);
    struct tm   t;
    char        buf[32];

    if (localtime_r(&now, &t) == NULL
        || strftime(buf, sizeof(buf), "%d/%m/%Y-%H:%M:%S", &t) == 0)
        return "00/00/0000-00:00:00";
    return std::string(buf);
}

// Replace control characters so a client can't forge log lines.
std::string Tintin_reporter::_clean(const std::string &s)
{
    std::string out(s);

    for (size_t i = 0; i < out.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(out[i]);
        if (c < 0x20 || c == 0x7f)
            out[i] = '?';
    }
    return out;
}

void Tintin_reporter::log(const std::string &level, const std::string &message)
{
    _file << "[" << _timestamp() << "] [ " << level << " ] - Matt_daemon: "
          << _clean(message) << std::endl;
}