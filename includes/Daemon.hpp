#ifndef DAEMON_HPP
#define DAEMON_HPP

#include <string>

#ifndef LOCK_FILE
# define LOCK_FILE "/var/lock/matt_daemon.lock"
#endif

class Daemon
{
public:
    Daemon();
    ~Daemon();
    bool daemonize();
    bool checkRoot() const;   // true if running as root
    bool lock();              // true if we got the lock, false if another instance runs
    void unlock();            // release the lock and remove the file
    static int pendingSignal();
private:
    Daemon(const Daemon &);
    Daemon &operator=(const Daemon &);

    int _lockFileFd;              // file descriptor of the lock file
};

#endif