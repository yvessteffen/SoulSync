#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <vector>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "Packet.hpp"

#pragma comment(lib, "ws2_32.lib")

constexpr int MAX_CLIENTS = 3;
constexpr int PORT = 5000;

struct Client {
    SOCKET sock = INVALID_SOCKET;
    uint8_t id;

    PacketHeader header;
    size_t headerReceived = 0;

    std::vector<uint8_t> buffer;
    size_t bytesReceived = 0;
};

std::vector<Client> clients;

void setNonBlocking(SOCKET sock)
{
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
}

void broadcast(const PacketHeader& header, const uint8_t* data, size_t size, uint8_t senderId)
{
    for (auto& c : clients)
    {
        if (c.id == senderId) continue;

        send(c.sock, (const char*)&header, sizeof(header), 0);
        send(c.sock, (const char*)data, (int)size, 0);
    }

    std::cout << "Forwarding frame from " << (int)senderId
          << " size=" << size << "\n";

}

void acceptClient(SOCKET serverSock)
{
    sockaddr_in addr{};
    int len = sizeof(addr);

    SOCKET sock = accept(serverSock, (sockaddr*)&addr, &len);
    if (sock == INVALID_SOCKET) return;

    if (clients.size() >= MAX_CLIENTS)
    {
        std::cout << "Server full, rejecting client.\n";
        closesocket(sock);
        return;
    }

    setNonBlocking(sock);

    uint8_t id = (uint8_t)clients.size() + 1;
    Client c;
    c.sock = sock;
    c.id = id;
    clients.push_back(std::move(c));
    std::cout << "Client connected: " << (int)id << "\n";
}

void handleClient(Client& c)
{
    if (c.headerReceived < sizeof(PacketHeader))
    {
        uint8_t* hptr = reinterpret_cast<uint8_t*>(&c.header);

        int r = recv(
            c.sock,
            (char*)hptr + c.headerReceived,
            sizeof(PacketHeader) - c.headerReceived,
            0
        );

        if (r <= 0) return;

        c.headerReceived += r;

        if (c.headerReceived < sizeof(PacketHeader))
            return;
    }

    if (c.header.size == 0 || c.header.size > 5 * 1024 * 1024)
    {
        std::cout << "INVALID HEADER SIZE: " << c.header.size << "\n";

        c.headerReceived = 0;
        c.bytesReceived = 0;
        return;
    }

    if (c.buffer.size() != c.header.size)
    {
        c.buffer.resize(c.header.size);
        c.bytesReceived = 0;
    }

    int r = recv(
        c.sock,
        (char*)c.buffer.data() + c.bytesReceived,
        c.buffer.size() - c.bytesReceived,
        0
    );

    if (r == 0)
    {
        std::cout << "Client " << (int)c.id << " disconnected\n";
        closesocket(c.sock);
        c.sock = INVALID_SOCKET;
        return;
    }

    if (r < 0) return;

    c.bytesReceived += r;

    if (c.bytesReceived == c.buffer.size())
    {
        broadcast(c.header, c.buffer.data(), c.buffer.size(), c.id);

        c.headerReceived = 0;
        c.bytesReceived = 0;
    }

    if (r == SOCKET_ERROR)
    {
        int err = WSAGetLastError();

        if (err == WSAEWOULDBLOCK)
            return;

        std::cout << "Client disconnected\n";
        closesocket(c.sock);
        c.sock = INVALID_SOCKET;
        return;
    }
};

void discardClient(Client& c)
{
    if (c.sock != INVALID_SOCKET)
    {
        closesocket(c.sock);
        c.sock = INVALID_SOCKET;
    }
}

int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, MAX_CLIENTS);

    setNonBlocking(serverSock);

    std::cout << "Relay server running on port " << PORT << "\n";

    while (true)
    {
        acceptClient(serverSock);

        for (auto it = clients.begin(); it != clients.end(); )
        {
            if (it->sock == INVALID_SOCKET)
            {
                it = clients.erase(it);
                continue;
            }

            handleClient(*it);
            ++it;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSock, &readfds);
        for (auto& c : clients)
            if (c.sock != INVALID_SOCKET)
                FD_SET((SOCKET)c.sock, &readfds);

        timeval tv { 0, 1000 };
        select(0, &readfds, nullptr, nullptr, &tv);
    }

    WSACleanup();
    return 0;
}