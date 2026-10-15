#include "Daemon.hpp"
#include "Server.hpp"
#include "Tintin_reporter.hpp"
#include <iostream>
#include <unistd.h>
#include <sstream>

int main()
{
    Daemon daemon;

    if (!daemon.checkRoot())
    {
        std::cerr << "Matt_daemon: must be run as root" << std::endl;
        return 1;
    }
    if (!daemon.lock())
    {
        std::cerr << "Can't open :/var/lock/matt_daemon.lock" << std::endl;
        return 1;                       // log nothing, like the subject's example
    }
    try
    {
        Tintin_reporter reporter;       // before daemonize(), so it's created with a sane umask
        reporter.log("INFO", "Started.");

        reporter.log("INFO", "Creating server.");
        Server server(reporter);
        if (!server.start())
        {
            reporter.log("ERROR", "Could not create server.");
            reporter.log("INFO", "Quitting.");
            return 1;                   // ~Daemon() removes the lock
        }
        reporter.log("INFO", "Server created.");

        reporter.log("INFO", "Entering Daemon mode.");
        if (!daemon.daemonize())
        {
            reporter.log("ERROR", "daemonize failed.");
            return 1;
        }
        daemon.installSignals();

        std::ostringstream oss;
        oss << "started. PID: " << getpid() << ".";
        reporter.log("INFO", oss.str());

        server.run();
        reporter.log("INFO", "Quitting.");
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;                           // destructors run: sockets closed, lock removed
}