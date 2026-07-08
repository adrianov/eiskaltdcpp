/*
 * SOCKS5 connect, auth, and UDP associate for Socket.
 */

#include "stdinc.h"
#include "Socket.h"

#include "format.h"
#include "SettingsManager.h"
#include "TimerManager.h"

namespace dcpp {

namespace {

constexpr uint32_t SOCKS_TIMEOUT = 30000;

inline uint64_t timeLeft(uint64_t start, uint64_t timeout) {
    if(timeout == 0)
        return 0;
    const uint64_t now = GET_TICK();
    if(start + timeout < now)
        throw SocketException(_("Connection timeout"));
    return start + timeout - now;
}

} // namespace

void Socket::socksConnect(const string& aAddr, const string& aPort, uint32_t timeout) {
    if(SETTING(SOCKS_SERVER).empty() || SETTING(SOCKS_PORT) == 0)
        throw SocketException(_("The socks server failed establish a connection"));

    const uint64_t start = GET_TICK();

    connect(SETTING(SOCKS_SERVER), Util::toString(SETTING(SOCKS_PORT)));

    if(wait(timeLeft(start, timeout), WAIT_CONNECT) != WAIT_CONNECT)
        throw SocketException(_("The socks server failed establish a connection"));

    socksAuth(timeLeft(start, timeout));

    ByteVector connStr;
    connStr.push_back(5);
    connStr.push_back(1);
    connStr.push_back(0);

    if(BOOLSETTING(SOCKS_RESOLVE)) {
        connStr.push_back(3);
        connStr.push_back((uint8_t)aAddr.size());
        connStr.insert(connStr.end(), aAddr.begin(), aAddr.end());
    } else {
        connStr.push_back(1);
        const unsigned long addr = inet_addr(resolve(aAddr).c_str());
        const uint8_t* paddr = (uint8_t*)&addr;
        connStr.insert(connStr.end(), paddr, paddr + 4);
    }

    const uint16_t port = htons(static_cast<uint16_t>(Util::toInt(aPort)));
    const uint8_t* pport = (uint8_t*)&port;
    connStr.push_back(pport[0]);
    connStr.push_back(pport[1]);

    writeAll(&connStr[0], connStr.size(), timeLeft(start, timeout));

    if(readAll(&connStr[0], 10, timeLeft(start, timeout)) != 10)
        throw SocketException(_("The socks server failed establish a connection"));

    if(connStr[0] != 5 || connStr[1] != 0)
        throw SocketException(_("The socks server failed establish a connection"));

    in_addr sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.s_addr = *((unsigned long*)&connStr[4]);
    setIp(inet_ntoa(sock_addr));
}

void Socket::socksAuth(uint32_t timeout) {
    vector<uint8_t> connStr;
    const uint64_t start = GET_TICK();

    if(SETTING(SOCKS_USER).empty() && SETTING(SOCKS_PASSWORD).empty()) {
        connStr.push_back(5);
        connStr.push_back(1);
        connStr.push_back(0);

        writeAll(&connStr[0], 3, timeLeft(start, timeout));

        if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2)
            throw SocketException(_("The socks server failed establish a connection"));

        if(connStr[1] != 0)
            throw SocketException(_("The socks server requires authentication"));
    } else {
        connStr.push_back(5);
        connStr.push_back(1);
        connStr.push_back(2);
        writeAll(&connStr[0], 3, timeLeft(start, timeout));

        if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2)
            throw SocketException(_("The socks server failed establish a connection"));
        if(connStr[1] != 2)
            throw SocketException(_("The socks server doesn't support login / password authentication"));

        connStr.clear();
        connStr.push_back(1);
        connStr.push_back((uint8_t)SETTING(SOCKS_USER).length());
        connStr.insert(connStr.end(), SETTING(SOCKS_USER).begin(), SETTING(SOCKS_USER).end());
        connStr.push_back((uint8_t)SETTING(SOCKS_PASSWORD).length());
        connStr.insert(connStr.end(), SETTING(SOCKS_PASSWORD).begin(), SETTING(SOCKS_PASSWORD).end());

        writeAll(&connStr[0], connStr.size(), timeLeft(start, timeout));

        if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2)
            throw SocketException(_("Socks server authentication failed (bad login / password?)"));

        if(connStr[1] != 0)
            throw SocketException(_("Socks server authentication failed (bad login / password?)"));
    }
}

void Socket::socksUpdated() {
    udpServer.clear();
    udpPort.clear();

    if(SETTING(OUTGOING_CONNECTIONS) != SettingsManager::OUTGOING_SOCKS5)
        return;

    try {
        Socket s;
        s.setBlocking(false);
        s.connect(SETTING(SOCKS_SERVER), Util::toString(SETTING(SOCKS_PORT)));
        s.socksAuth(SOCKS_TIMEOUT);

        char connStr[10];
        connStr[0] = 5;
        connStr[1] = 3;
        connStr[2] = 0;
        connStr[3] = 1;
        *((uint32_t*)(&connStr[4])) = 0;
        *((uint16_t*)(&connStr[8])) = 0;

        s.writeAll(connStr, 10, SOCKS_TIMEOUT);

        if(s.readAll(connStr, 10, SOCKS_TIMEOUT) != 10)
            return;

        if(connStr[0] != 5 || connStr[1] != 0)
            return;

        udpPort = Util::toString(ntohs(*((uint16_t*)(&connStr[8]))));

        in_addr serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.s_addr = *((long*)(&connStr[4]));
        udpServer = inet_ntoa(serv_addr);
    } catch(const SocketException&) {
        dcdebug("Socket: Failed to register with socks server\n");
    }
}

} // namespace dcpp
