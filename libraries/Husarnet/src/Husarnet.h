#ifndef Husarnet_h
#define Husarnet_h

#include "Arduino.h"
#include "HusarnetServer.h"
#include "IPv6Address.h"
#include "WString.h"
#include <vector>
#include <utility>
#include <string>

struct _Husarnet
{
    // Sets up Husarnet to use self-hosted base server
    void selfHostedSetup(const char *hostname);

    // Starts the Husarnet
    void start();

    // Provides join code. Use before Husarnet.start().
    void join(const char *joinCode, const char *hostname = "");

    // Get list of peers' hostnames and addresses
    std::vector<std::pair<IPv6Address, String>> listPeers();

    // Get hostname you're currently known at
    String getHostname();
};

extern _Husarnet Husarnet;

#endif
