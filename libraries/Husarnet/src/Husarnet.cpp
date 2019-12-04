#include "Husarnet.h"
#include <cassert>

_Husarnet Husarnet;

extern "C" {
    void husarnet_start();
    void husarnet_join(const char* joinCode, const char* hostname);
}

static bool alreadyStarted = false;

void _Husarnet::start() {
    assert(!alreadyStarted);
    alreadyStarted = true;
    husarnet_start();
}

void _Husarnet::join(const char* joinCode, const char* hostname) {
    assert(!alreadyStarted);
    husarnet_join(joinCode, hostname);
}
