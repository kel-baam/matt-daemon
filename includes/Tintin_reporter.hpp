#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

#include <string>
#include <fstream>

#define LOG_DIR  "/var/log/matt_daemon"
#define LOG_FILE "/var/log/matt_daemon/matt_daemon.log"

class Tintin_reporter
{
public:
    Tintin_reporter();
    ~Tintin_reporter();

    void log(const std::string &level, const std::string &message);

private:
    Tintin_reporter(const Tintin_reporter &);
    Tintin_reporter &operator=(const Tintin_reporter &);

    std::string _timestamp() const;
    static std::string _clean(const std::string &s);

    std::ofstream _file;
};

#endif