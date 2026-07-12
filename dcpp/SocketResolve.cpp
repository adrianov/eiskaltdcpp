/*
 * DNS resolve, TCP connect, and select-based wait for Socket.
 */

#include "stdinc.h"
#include "Socket.h"

#include "format.h"

namespace dcpp {

void Socket::connect(const string& aAddr, const string& aPort, const string&) {
    sockaddr_in serv_addr;

    if(sock == INVALID_SOCKET) {
        create(TYPE_TCP);
    }

    string addr = resolve(aAddr);
    if(addr.empty())
        throw SocketException(str(F_("Unable to resolve %1%") % aAddr));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_port = htons(static_cast<uint16_t>(Util::toInt(aPort)));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addr.c_str());

    int result;
    do {
        result = ::connect(sock,(sockaddr*)&serv_addr,sizeof(serv_addr));
    } while (result < 0 && getLastError() == EINTR);
    check(result, true);

    connected = true;
    setIp(addr);
}

/**
 * Blocks until timeout is reached one of the specified conditions have been fulfilled
 * @param millis Max milliseconds to block.
 * @param waitFor WAIT_*** flags that set what we're waiting for, set to the combination of flags that
 *                triggered the wait stop on return (==WAIT_NONE on timeout)
 * @return WAIT_*** ored together of the current state.
 * @throw SocketException Select or the connection attempt failed.
 */
int Socket::wait(uint32_t millis, int waitFor) {
    timeval tv;
    fd_set rfd, wfd, efd;
    fd_set *rfdp = NULL, *wfdp = NULL;
    tv.tv_sec = millis/1000;
    tv.tv_usec = (millis%1000)*1000;

    if(waitFor & WAIT_CONNECT) {
        dcassert(!(waitFor & WAIT_READ) && !(waitFor & WAIT_WRITE));

        int result;
        do {
            FD_ZERO(&wfd);
            FD_ZERO(&efd);

            FD_SET(sock, &wfd);
            FD_SET(sock, &efd);
            result = select((int)(sock+1), 0, &wfd, &efd, &tv);
        } while (result < 0 && getLastError() == EINTR);
        check(result);

        // fix buffer overflow during shutdown
        if(sock == INVALID_SOCKET)
            return WAIT_NONE;

        if(FD_ISSET(sock, &wfd)) {
            return WAIT_CONNECT;
        }

        if(FD_ISSET(sock, &efd)) {
            int y = 0;
            socklen_t z = sizeof(y);
            check(getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&y, &z));

            if(y != 0)
                throw SocketException(y);
            // No errors! We're connected (?)...
            return WAIT_CONNECT;
        }
        return WAIT_NONE;
    }

    int result;
    do {
        if(waitFor & WAIT_READ) {
            dcassert(!(waitFor & WAIT_CONNECT));
            rfdp = &rfd;
            FD_ZERO(rfdp);
            FD_SET(sock, rfdp);
        }
        if(waitFor & WAIT_WRITE) {
            dcassert(!(waitFor & WAIT_CONNECT));
            wfdp = &wfd;
            FD_ZERO(wfdp);
            FD_SET(sock, wfdp);
        }

        result = select((int)(sock+1), rfdp, wfdp, NULL, &tv);
    } while (result < 0 && getLastError() == EINTR);
    check(result);

    waitFor = WAIT_NONE;

    // fix buffer overflow during shutdown
    if(sock == INVALID_SOCKET)
        return WAIT_NONE;

    if(rfdp && FD_ISSET(sock, rfdp)) {
        waitFor |= WAIT_READ;
    }
    if(wfdp && FD_ISSET(sock, wfdp)) {
        waitFor |= WAIT_WRITE;
    }

    return waitFor;
}

bool Socket::waitConnected(uint32_t millis) {
    return wait(millis, Socket::WAIT_CONNECT) == WAIT_CONNECT;
}

bool Socket::waitAccepted(uint32_t millis) {
    (void)millis;
    // Normal sockets are always connected after a call to accept
    return true;
}

string Socket::resolve(const string& aDns) {
#ifdef _WIN32
    sockaddr_in sock_addr;

    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_port = 0;
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(aDns.c_str());

    if (sock_addr.sin_addr.s_addr == INADDR_NONE) {   /* server address is a name or invalid */
        hostent* host;
        host = gethostbyname(aDns.c_str());
        if (host == NULL) {
            return Util::emptyString;
        }
        sock_addr.sin_addr.s_addr = *((uint32_t*)host->h_addr);
        return inet_ntoa(sock_addr.sin_addr);
    } else {
        return aDns;
    }
#else
    // POSIX doesn't guarantee the gethostbyname to be thread safe. And it may (will) return a pointer to static data.
    string address = Util::emptyString;
    addrinfo hints = { 0, 0, 0, 0, 0, 0, 0, 0 };
    addrinfo *result;
    // While we do not have IPv6 support, hints.ai_family = AF_UNSPEC causes connection problem
    // See: https://code.google.com/p/eiskaltdc/issues/detail?id=1417
    hints.ai_family = AF_INET;    /* Allow only IPv4 */
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    //hints.ai_flags = AI_IDN | AI_CANONIDN;// | AI_IDN_ALLOW_UNASSIGNED;

    if (getaddrinfo(aDns.c_str(), NULL, &hints, &result) == 0) {
        if (result->ai_addr != NULL)
            address = inet_ntoa(((sockaddr_in*)(result->ai_addr))->sin_addr);

        freeaddrinfo(result);
    }
    return address;
#endif
}

} // namespace dcpp
