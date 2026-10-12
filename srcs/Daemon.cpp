#include "Daemon.hpp"
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

Daemon::Daemon() : _lockFileFd(-1) {}

Daemon::~Daemon()
{
    unlock();
}

bool Daemon::checkRoot() const
{
    return geteuid() == 0;
}

bool Daemon::lock()
{
    _lockFileFd = open(LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (_lockFileFd < 0)
        return false;
    if (flock(_lockFileFd, LOCK_EX | LOCK_NB) < 0)
    {
        close(_lockFileFd);
        _lockFileFd = -1;
        return false;
    }
    return true;
}

void Daemon::unlock()
{
    if (_lockFileFd >= 0)
    {
        unlink(LOCK_FILE);   // remove the file first, while we still hold the lock
        flock(_lockFileFd, LOCK_UN);
        close(_lockFileFd);
        _lockFileFd = -1;
    }
}


bool Daemon::daemonize()
{
    pid_t pid = fork();

    if (pid < 0)
        return false;

    if (pid > 0)
        exit(0);

    if (setsid() < 0)
        return false;

    pid = fork();

    if (pid < 0)
        return false;

    if (pid > 0)
        exit(0);

    umask(0);

    if (chdir("/") < 0)
        return false;

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return true;
}