#include "Tintin_reporter.hpp"
#include "Daemon.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

int main()
{
    Daemon daemon;

    // 1. Root check (before anything else)
    if (!daemon.checkRoot())
    {
        std::cerr << "Matt_daemon: must be run as root" << std::endl;
        return 1;
    }

    try
    {
        // 2. Logger
        Tintin_reporter reporter;
        reporter.log("INFO", "Started.");

        // 3. Lock (must happen BEFORE daemonize, so the error reaches stderr)
        if (!daemon.lock())
        {
            std::cerr << "Can't open :" << LOCK_FILE << std::endl;
            reporter.log("ERROR", "Error file locked.");
            return 1;
        }

        // 4. Server (also before daemonize, so a port error is visible)
        reporter.log("INFO", "Creating server.");
        Server server(reporter);
        if (!server.start())
        {
            std::cerr << "Can't start server on port 4242" << std::endl;
            reporter.log("ERROR", "Server creation failed.");
            return 1;
        }
        reporter.log("INFO", "Server created.");

        // 5. Go into the background
        reporter.log("INFO", "Entering Daemon mode.");
        if (!daemon.daemonize())
        {
            reporter.log("ERROR", "Daemonize failed.");
            return 1;
        }

        std::ostringstream oss;
        oss << "started. PID: " << getpid() << ".";
        reporter.log("INFO", oss.str());

        // 6. Signals, then the main loop
        daemon.setupSignals();
        server.run();

        // 7. Reached on "quit" or on a signal
        reporter.log("INFO", "Quitting.");
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}