/*
  Client.h - Client class for Raspberry Pi
  Copyright (c) 2016 Hristo Gochkov  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "HusarnetClient.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>
#include "esp_log.h"

#define WIFI_CLIENT_MAX_WRITE_RETRY   (10)
#define WIFI_CLIENT_SELECT_TIMEOUT_US (1000000)
#define WIFI_CLIENT_FLUSH_BUFFER_SIZE (1024)

#undef connect
#undef write
#undef read

class HusarnetClientSocketHandle {
private:
    int sockfd;

public:
    HusarnetClientSocketHandle(int fd):sockfd(fd)
    {
    }

    ~HusarnetClientSocketHandle()
    {
        close(sockfd);
    }

    int fd()
    {
        return sockfd;
    }
};

HusarnetClient::HusarnetClient():_connected(false),next(NULL)
{
}

HusarnetClient::HusarnetClient(int fd):_connected(true),next(NULL)
{
    clientSocketHandle.reset(new HusarnetClientSocketHandle(fd));
}

HusarnetClient::~HusarnetClient()
{
    stop();
}

HusarnetClient & HusarnetClient::operator=(const HusarnetClient &other)
{
    stop();
    clientSocketHandle = other.clientSocketHandle;
    _connected = other._connected;
    return *this;
}

void HusarnetClient::stop()
{
    clientSocketHandle = NULL;
    _connected = false;
}

int HusarnetClient::connect(IPAddress ip, uint16_t port)
{
    ESP_LOGE("husarnet", "Husarnet doesn't support IPv4 addresses");
    return 0;
}

int HusarnetClient::connect(IP6Address ip, uint16_t port)
{
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_e("socket: %d", errno);
        return 0;
    }

    struct sockaddr_in6 serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin6_family = AF_INET6;
    serveraddr.sin6_port = htons(port);
    serveraddr.sin6_len = sizeof(serveraddr);
    memcpy(&serveraddr.sin6_addr, &ip.data[0], 16);

    int res = lwip_connect_r(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (res < 0) {
        log_e("lwip_connect_r: %d", errno);
        close(sockfd);
        return 0;
    }
    clientSocketHandle.reset(new HusarnetClientSocketHandle(sockfd));
    _connected = true;
    return 1;
}

// provided by WebSetup
sockaddr_storage resolve(const std::string& hostname, const std::string& port);

int HusarnetClient::connect(const char *host, uint16_t port)
{
    sockaddr_storage ss = resolve(host, "1");

    if (ss.ss_family != AF_INET6) {
        return 0;
    }
    IP6Address srv;
    memcpy(&srv.data[0], &((sockaddr_in6*)&ss)->sin6_addr, 16);
    return connect(srv, port);
}

int HusarnetClient::setSocketOption(int option, char* value, size_t len)
{
    int res = setsockopt(fd(), SOL_SOCKET, option, value, len);
    if(res < 0) {
        log_e("%X : %d", option, errno);
    }
    return res;
}

int HusarnetClient::setTimeout(uint32_t seconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if(setSocketOption(SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0) {
        return -1;
    }
    return setSocketOption(SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
}

int HusarnetClient::setOption(int option, int *value)
{
    int res = setsockopt(fd(), IPPROTO_TCP, option, (char *) value, sizeof(int));
    if(res < 0) {
        log_e("%d", errno);
    }
    return res;
}

int HusarnetClient::getOption(int option, int *value)
{
    size_t size = sizeof(int);
    int res = getsockopt(fd(), IPPROTO_TCP, option, (char *)value, &size);
    if(res < 0) {
        log_e("%d", errno);
    }
    return res;
}

int HusarnetClient::setNoDelay(bool nodelay)
{
    int flag = nodelay;
    return setOption(TCP_NODELAY, &flag);
}

bool HusarnetClient::getNoDelay()
{
    int flag = 0;
    getOption(TCP_NODELAY, &flag);
    return flag;
}

size_t HusarnetClient::write(uint8_t data)
{
    return write(&data, 1);
}

int HusarnetClient::read()
{
    uint8_t data = 0;
    int res = read(&data, 1);
    if(res < 0) {
        return res;
    }
    return data;
}

size_t HusarnetClient::write(const uint8_t *buf, size_t size)
{
    int res =0;
    int retry = WIFI_CLIENT_MAX_WRITE_RETRY;
    int socketFileDescriptor = fd();
    size_t totalBytesSent = 0;
    size_t bytesRemaining = size;

    if(!_connected || (socketFileDescriptor < 0)) {
        return 0;
    }

    while(retry) {
        //use select to make sure the socket is ready for writing
        fd_set set;
        struct timeval tv;
        FD_ZERO(&set);        // empties the set
        FD_SET(socketFileDescriptor, &set); // adds FD to the set
        tv.tv_sec = 0;
        tv.tv_usec = WIFI_CLIENT_SELECT_TIMEOUT_US;
        retry--;

        if(select(socketFileDescriptor + 1, NULL, &set, NULL, &tv) < 0) {
            return 0;
        }

        if(FD_ISSET(socketFileDescriptor, &set)) {
            res = send(socketFileDescriptor, (void*) buf, bytesRemaining, MSG_DONTWAIT);
            if(res > 0) {
                totalBytesSent += res;
                if (totalBytesSent >= size) {
                    //completed successfully
                    retry = 0;
                } else {
                    buf += res;
                    bytesRemaining -= res;
                }
            }
            else if(res < 0) {
                log_e("%d", errno);
                if(errno != EAGAIN) {
                    //if resource was busy, can try again, otherwise give up
                    stop();
                    res = 0;
                    retry = 0;
                }
            }
            else {
                // Try again
            }
        }
    }
    return totalBytesSent;
}

size_t HusarnetClient::write_P(PGM_P buf, size_t size)
{
    return write(buf, size);
}

int HusarnetClient::read(uint8_t *buf, size_t size)
{
    if(!available()) {
        return -1;
    }
    int res = recv(fd(), buf, size, MSG_DONTWAIT);
    if(res < 0 && errno != EWOULDBLOCK) {
        log_e("%d", errno);
        stop();
    }
    return res;
}

int HusarnetClient::peek()
{
    if(!available()) {
        return -1;
    }
    uint8_t data = 0;
    int res = recv(fd(), &data, 1, MSG_PEEK);
    if(res < 0 && errno != EWOULDBLOCK) {
        log_e("%d", errno);
        stop();
    }
    return data;
}

int HusarnetClient::available()
{
    if(!_connected) {
        return 0;
    }
    int count;
    int res = lwip_ioctl_r(fd(), FIONREAD, &count);
    if(res < 0) {
        log_e("%d", errno);
        stop();
        return 0;
    }
    return count;
}

// Though flushing means to send all pending data,
// seems that in Arduino it also means to clear RX
void HusarnetClient::flush() {
    int res;
    size_t a = available(), toRead = 0;
    if(!a){
        return;//nothing to flush
    }
    uint8_t * buf = (uint8_t *)malloc(WIFI_CLIENT_FLUSH_BUFFER_SIZE);
    if(!buf){
        return;//memory error
    }
    while(a){
        toRead = (a>WIFI_CLIENT_FLUSH_BUFFER_SIZE)?WIFI_CLIENT_FLUSH_BUFFER_SIZE:a;
        res = recv(fd(), buf, toRead, MSG_DONTWAIT);
        if(res < 0) {
            log_e("%d", errno);
            stop();
            break;
        }
        a -= res;
    }
    free(buf);
}

uint8_t HusarnetClient::connected()
{
    if (_connected) {
        uint8_t dummy;
        int res = recv(fd(), &dummy, 0, MSG_DONTWAIT);
        if (res <= 0) {
            switch (errno) {
                case ENOTCONN:
                case EPIPE:
                case ECONNRESET:
                case ECONNREFUSED:
                case ECONNABORTED:
                    _connected = false;
                    break;
                default:
                    _connected = true;
                    break;
            }
        }
        else {
            // Should never happen since requested 0 bytes
            _connected = true;
        }
    }
    return _connected;
}

bool HusarnetClient::operator==(const HusarnetClient& rhs)
{
    return clientSocketHandle == rhs.clientSocketHandle;
}

int HusarnetClient::fd() const
{
    if (clientSocketHandle == NULL) {
        return -1;
    } else {
        return clientSocketHandle->fd();
    }
}

