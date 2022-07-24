/*
 * Copyright ©2022 Travis McGaha.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Pennsylvania
 * CIT 595 for use solely during Spring Semester 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <cstdio>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <cstring>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

namespace searchserver {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}
// Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"
  // TODO: implement
bool ServerSocket::bind_and_listen(int ai_family, int *listen_fd) {

  sock_family_=ai_family;
  // create default struct
  struct addrinfo hints = {0};
  memset(&hints, 0 ,sizeof(struct addrinfo));
  hints.ai_family = ai_family;
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use an address we can bind to a socket and accept client connections on
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;
  struct addrinfo *result;

  char service_name [16];
  sprintf (service_name, "%u", (unsigned int) port_);
  // std::cout << service_name << std::endl;

  int res = getaddrinfo(nullptr, service_name, &hints, &result);

  // check if getaddrinfo failed
  if(res != 0) {
    perror("getaddrinfo() failed");
    return false;
  }
  // std::cout << res << std::endl;

  // loop through returned addr struct until we can create a socket and bind to one
  *listen_fd = -1;
  for(struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    *listen_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if(*listen_fd == -1) {
      listen_fd = 0;
      continue;
    }
    // configure socket
    int optval = 1;
    setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // try binding socket to addr and port num returned by getaddrinfo
    if(bind(*listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // bind success
      break;
    }
    // bind failed. close socket and continue to loop
    close(*listen_fd);
    *listen_fd = -1;
  }

  // free struct
  freeaddrinfo(result);

  // check if binds to an addr
  if(*listen_fd == -1) {
    return EXIT_FAILURE;
  }

  // tell OS this will be listening socket
  if(listen(*listen_fd, SOMAXCONN) != 0) {
    close(*listen_fd);
    return EXIT_FAILURE;
  }

  listen_sock_fd_ = *listen_fd;
  return true;
}

bool ServerSocket::accept_client(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dns_name,
                          std::string *server_addr,
                          std::string *server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  if(listen_sock_fd_<= 0){
    perror("Failed to bind to addr");
    return EXIT_FAILURE;
  }
  // std::cout << "Server socket: "<< listen_sock_fd_ << std::endl;

  int client_socket;
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  memset(&caddr, '\0', caddr_len);

  while(1) {
    // accepting client depending whether it's ipv4 or ipv6
    client_socket = accept(listen_sock_fd_, reinterpret_cast<struct sockaddr*>(&caddr), &caddr_len);
    *accepted_fd = client_socket;
    // std::cout << "Client socket: "<< client_socket << std::endl;

    if(client_socket < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
    }
  

  // either client name is inet_ntop should = dns ; else hname (first char to be null) getnameinfo fails then dns = hname
  // create variable to hold client_dns_name : peer (sockaddr*)
  // sockaddr* peer_addr;
  // socklen_t peer_addrlen = sizeof(peer_addr);
  // getnameinfo should be in each type of AF_INET sockaddr_in & sock_addr_in6
  
  char hname[1024];
  hname[0] = '\0';

  // GET CLIENT ADDR 
  if (caddr.ss_family == AF_INET) {
      // IPV4 address 
      struct sockaddr_in* sa = (struct sockaddr_in*) &caddr;
      char addrbuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &sa->sin_addr, addrbuf , INET_ADDRSTRLEN);
      *client_addr = addrbuf;
      *client_port = ntohs(sa->sin_port);

      getnameinfo((const struct sockaddr *) sa, sizeof(*sa), hname, 1024, nullptr, 0, 0);
          // getnameinfo failed
          // reinterpret_cast on hname
          *client_dns_name = std::string(hname);
        
    } else {
      // IPV6 address and port
      struct sockaddr_in6* sa = (struct sockaddr_in6*) &caddr;
      char addrbuf[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &sa->sin6_addr, addrbuf, INET6_ADDRSTRLEN);
      *client_addr = addrbuf;
      *client_port = ntohs(sa->sin6_port);

      getnameinfo((const struct sockaddr *) sa, sizeof(*sa), hname, 1024, nullptr, 0, 0);
      *client_dns_name = std::string(hname);
    }

    char hname1[1024];
    hname1[0] = '\0';

  // GET SERVER ADDR
  if (sock_family_ == AF_INET) {
    struct sockaddr_in srvr;
    // The server is using an IPv4 address.
    socklen_t srvrlen = sizeof(srvr);
    getsockname(*accepted_fd, (struct sockaddr *) &srvr, &srvrlen);
    char addrbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    *server_addr = addrbuf;
   
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname1, 1024, nullptr, 0, 0);
    *server_dns_name = std::string(hname1);
    
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(*accepted_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    *server_addr = addrbuf;
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname1, 1024, nullptr, 0, 0);
    *server_dns_name = std::string(hname1);
  }
  break;
}
  return true;
}

}  // namespace searchserver
