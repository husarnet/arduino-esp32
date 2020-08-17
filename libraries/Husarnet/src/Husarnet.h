#ifndef Husarnet_h
#define Husarnet_h

#include "Arduino.h"
#include "HusarnetServer.h"
//#include <freertos/task.h>

struct _Husarnet {
    // Sets up Husarnet to use self-hosted base server
    void selfHostedSetup(const char* hostname);

    // Starts the Husarnet
    void start();

    // Provides join code. Use before Husarnet.start().
    void join(const char* joinCode, const char* hostname="");
};

extern _Husarnet Husarnet;

#endif
