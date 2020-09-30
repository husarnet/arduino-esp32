#include "Husarnet.h"
#include <cassert>


_Husarnet Husarnet;

extern "C" {
    void husarnet_start();
    void husarnet_join(const char* joinCode, const char* hostname);
    void husarnet_retrieve_license(const char* hostname);
    const char* husarnet_get_hostname();
}

struct InternalIP6Address {
    std::array<uint8_t, 16> data;
};

extern std::vector<std::pair<InternalIP6Address, std::string>> husarnet_hosts;

static bool alreadyStarted = false;

void _Husarnet::selfHostedSetup(const char* hostname) {
    assert(!alreadyStarted);
    husarnet_retrieve_license(hostname);
}

void _Husarnet::start() {
    assert(!alreadyStarted);
    alreadyStarted = true;
    husarnet_start();
}

void _Husarnet::join(const char* joinCode, const char* hostname) {
    assert(!alreadyStarted);
    husarnet_join(joinCode, hostname);
}

typedef std::pair<IPv6Address, String> hostPair;
std::vector<hostPair> _Husarnet::listPeers() {
    std::vector<hostPair> peers;

    for(auto const& host: husarnet_hosts) {
        peers.push_back(hostPair(IPv6Address(host.first.data.data()), String(host.second.c_str())));
    }

    return peers;
}

String _Husarnet::getHostname() {
    return String(husarnet_get_hostname());
}
