#include "Daemon.hpp"
#include "Tintin_reporter.hpp"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static volatile sig_atomic_t g_sig = 0;

// Normal signals: just remember it, the server loop logs it and quits.
static void softHandler(int sig)
{
    g_sig = sig;
}

// Synchronous faults can't "return": the faulting instruction would rerun
// forever. Best-effort log + cleanup, then die.
static void fatalHandler(int sig)
{
    char    buf[128];
    char    ts[32];
    time_t  now = time(NULL);
    struct tm t;

    ts[0] = '\0';
    if (localtime_r(&now, &t))
        strftime(ts, sizeof(ts), "%d/%m/%Y-%H:%M:%S", &t);
    int len = snprintf(buf, sizeof(buf),
        "[%s] [ ERROR ] - Matt_daemon: Fatal signal %d received. Quitting.\n",
        ts, sig);
    int fd = open(LOG_FILE, O_WRONLY | O_APPEND);
    if (fd >= 0)
    {
        if (len > 0)
        {
            ssize_t r = write(fd, buf, static_cast<size_t>(len));
            (void)r;
        }
        close(fd);
    }
    unlink(LOCK_FILE);
    _exit(128 + sig);
}

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
    _lockFileFd = open(LOCK_FILE, O_CREAT | O_RDWR | O_CLOEXEC, 0644);
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
        unlink(LOCK_FILE);   // remove while we still hold the lock
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
        _exit(0);               // _exit: no destructors, the lock stays ours

    if (setsid() < 0)
        return false;

    pid = fork();
    if (pid < 0)
        return false;
    if (pid > 0)
        _exit(0);

    umask(022);
    if (chdir("/") < 0)
        return false;

    int fd = open("/dev/null", O_RDWR);
    if (fd < 0)
        return false;
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO)
        close(fd);
    return true;
}

void Daemon::installSignals()
{
    struct sigaction soft;
    struct sigaction fatal;

    memset(&soft, 0, sizeof(soft));
    soft.sa_handler = softHandler;
    sigemptyset(&soft.sa_mask);

    memset(&fatal, 0, sizeof(fatal));
    fatal.sa_handler = fatalHandler;
    sigemptyset(&fatal.sa_mask);

    for (int s = 1; s < NSIG; s++)
    {
        // SIGKILL and SIGSTOP can't be caught; the others are harmless.
        if (s == SIGKILL || s == SIGSTOP || s == SIGCHLD
            || s == SIGWINCH || s == SIGURG || s == SIGCONT)
            continue;
        if (s == SIGSEGV || s == SIGFPE || s == SIGILL || s == SIGBUS)
            sigaction(s, &fatal, NULL);
        else
            sigaction(s, &soft, NULL);   // no SA_RESTART: poll() gets EINTR
    }
}

int Daemon::pendingSignal()
{
    return g_sig;
}