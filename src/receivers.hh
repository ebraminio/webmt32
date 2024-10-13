#pragma once

#include <cstdint>
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
    sockaddr_in cliaddr{}, servaddr{};
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
        fd_set fds{};
        FD_SET(listenfd, &fds);
        FD_SET(udpfd, &fds);

        timeval timeout{};
        select(maxfdp1, &fds, nullptr, nullptr, &timeout);

        if (!FD_ISSET(udpfd, &fds)) return 0;
        return recvfrom(udpfd, buffer, size, 0, reinterpret_cast<sockaddr *>(&cliaddr), &len);
    }
};

#include <termios.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

// This is derived from https://gist.github.com/gdamjan/5544239 but highly changed
/* Copyright (C) 2007 Ivan Tikhonov
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  Ivan Tikhonov, kefeer@brokestream.com
*/
class TTYReceiver final : public Receiver {
    termios oldtio{}, newtio{};
    int comfd;

public:
    explicit TTYReceiver(const char *deviceName) {
        comfd = open(deviceName, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (comfd < 0) {
            perror(deviceName);
            exit(-1);
        }

        tcgetattr(comfd, &oldtio); // save current port settings
        int speed = B38400;
        newtio.c_cflag = speed | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VMIN] = 1;
        newtio.c_cc[VTIME] = 0;
        tcflush(comfd, TCIFLUSH);
        tcsetattr(comfd, TCSANOW, &newtio);
    }

    unsigned receive(uint8_t *buffer, const int size) override {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(comfd, &fds);

        select(comfd + 1, &fds, nullptr, nullptr, nullptr);
        if (!FD_ISSET(comfd, &fds)) return 0;

        return read(comfd, &buffer, size);
    }

    ~TTYReceiver() override {
        tcsetattr(comfd, TCSANOW, &oldtio);
        close(comfd);
    }
};
