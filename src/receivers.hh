#pragma once

#include <cstdint>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct Receiver {
    virtual ~Receiver() = default;

    virtual unsigned receive(
        [[maybe_unused]] uint8_t *buffer,
        [[maybe_unused]] int size
    ) { return 0; }
};

// This initially was based on https://www.geeksforgeeks.org/tcp-and-udp-server-using-select/
class UDPReceiver final : public Receiver {
    int listenfd, udpfd, maxfdp1;
    fd_set rset = {};
    sockaddr_in cliaddr = {}, servaddr = {};
    socklen_t len = sizeof cliaddr;

public:
    explicit UDPReceiver(unsigned port) {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        bind(listenfd, reinterpret_cast<sockaddr *>(&servaddr), sizeof servaddr);
        listen(listenfd, 10);

        udpfd = socket(AF_INET, SOCK_DGRAM, 0);
        bind(udpfd, reinterpret_cast<sockaddr *>(&servaddr), sizeof servaddr);
        maxfdp1 = (listenfd > udpfd ? listenfd : udpfd) + 1;
    }

    unsigned receive(uint8_t *buffer, const int size) override {
        FD_SET(listenfd, &rset);
        FD_SET(udpfd, &rset);

        timeval timeout = {};
        select(maxfdp1, &rset, nullptr, nullptr, &timeout);

        if (!FD_ISSET(udpfd, &rset)) return 0;
        return recvfrom(udpfd, buffer, size, 0, reinterpret_cast<sockaddr *>(&cliaddr), &len);
    }
};
