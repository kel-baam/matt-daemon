#ifndef DAEMON_HPP
#define DAEMON_HPP

#define LOCK_FILE "/var/lock/matt_daemon.lock"

class Daemon
{
public:
    Daemon();
    ~Daemon();

    bool checkRoot() const;
    bool lock();
    void unlock();
    bool daemonize();

    static void installSignals();
    static int  pendingSignal();

private:
    Daemon(const Daemon &);
    Daemon &operator=(const Daemon &);

    int _lockFileFd;
};

#endif