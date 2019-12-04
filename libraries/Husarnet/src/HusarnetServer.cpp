/*
  Server.cpp - Server class for Raspberry Pi
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
#include "HusarnetServer.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "esp_log.h"

#undef write
#undef close

int HusarnetServer::setTimeout(uint32_t seconds){
  struct timeval tv;
  tv.tv_sec = seconds;
  tv.tv_usec = 0;
  if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
    return -1;
  return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
}

size_t HusarnetServer::write(const uint8_t *data, size_t len){
  return 0;
}

void HusarnetServer::stopAll(){}

HusarnetClient HusarnetServer::available(){
  if(!_listening)
    return HusarnetClient();
  int client_sock;

  if (_accepted_sockfd >= 0) {
    client_sock = _accepted_sockfd;
    _accepted_sockfd = -1;
  }
  else {
    struct sockaddr_in6 _client;
    int cs = sizeof(struct sockaddr_in6);
    client_sock = lwip_accept_r(sockfd, (struct sockaddr *)&_client, (socklen_t*)&cs);
  }
  if(client_sock >= 0){
    int val = 1;
    if(setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(int)) == ESP_OK) {
      val = _noDelay;
      if(setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(int)) == ESP_OK)
        return HusarnetClient(client_sock);
    }
  }
  return HusarnetClient();
}

void HusarnetServer::begin(){
  if(_listening)
    return;
  struct sockaddr_in6 server;
  sockfd = lwip_socket(AF_INET6, SOCK_STREAM, 0);
  if (sockfd < 0)
    return;
  memset(&server, 0, sizeof(sockaddr_in6));
  server.sin6_family = AF_INET6;
  server.sin6_port = htons(_port);
  server.sin6_len = sizeof(server);

  if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    return;
  if(listen(sockfd , _max_clients) < 0)
    return;
  fcntl(sockfd, F_SETFL, O_NONBLOCK);

  _listening = true;
  _noDelay = false;
  _accepted_sockfd = -1;
}

void HusarnetServer::setNoDelay(bool nodelay) {
    _noDelay = nodelay;
}

bool HusarnetServer::getNoDelay() {
    return _noDelay;
}

bool HusarnetServer::hasClient() {
    if (_accepted_sockfd >= 0) {
      return true;
    }
    struct sockaddr_in _client;
    int cs = sizeof(struct sockaddr_in);
    _accepted_sockfd = lwip_accept_r(sockfd, (struct sockaddr *)&_client, (socklen_t*)&cs);
    if (_accepted_sockfd >= 0) {
      return true;
    }
    return false;
}

void HusarnetServer::end(){
  lwip_close_r(sockfd);
  sockfd = -1;
  _listening = false;
}

void HusarnetServer::close(){
  end();
}

void HusarnetServer::stop(){
  end();
}

