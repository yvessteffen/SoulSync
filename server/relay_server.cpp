#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <vector>
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int MAX_CLIENTS = 3;
constexpr int PORT = 5000;

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t type;
    uint8_t senderId;
    uint32_t size;
};
#pragma pack(pop)

struct Client {
    int sock;
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
}

void acceptClient(SOCKET serverSock)
{
    sockaddr_in addr{};
    int len = sizeof(addr);

    SOCKET sock = accept(serverSock, (sockaddr*)&addr, &len);
    if (sock == INVALID_SOCKET) return;

    if (clients.size() >= MAX_CLIENTS)
    {
        closesocket(sock);
        return;
    }

    setNonBlocking(sock);

    Client c;
    c.sock = sock;
    c.id = (uint8_t)clients.size();

    clients.push_back(std::move(c));

    std::cout << "Client connected: " << (int)c.id << "\n";
}

void handleClient(Client& c)
{
    while (true)
    {
        // ---------- HEADER ----------
        if (c.headerReceived < sizeof(PacketHeader))
        {
            char* hptr = (char*)&c.header;

            int r = recv(
                c.sock,
                hptr + c.headerReceived,
                (int)(sizeof(PacketHeader) - c.headerReceived),
                0
            );

            if (r == 0) return; // disconnect
            if (r == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) return;
                return;
            }

            c.headerReceived += r;

            if (c.headerReceived < sizeof(PacketHeader))
                return;
        }

        // ---------- PAYLOAD ----------
        if (c.buffer.size() != c.header.size)
        {
            c.buffer.resize(c.header.size);
            c.bytesReceived = 0;
        }

        int r = recv(
            c.sock,
            (char*)c.buffer.data() + c.bytesReceived,
            (int)(c.buffer.size() - c.bytesReceived),
            0
        );

        if (r == 0) return;
        if (r == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) return;
            return;
        }

        c.bytesReceived += r;

        // ---------- FULL FRAME ----------
        if (c.bytesReceived == c.buffer.size())
        {
            c.header.senderId = c.id;

            broadcast(c.header, c.buffer.data(), c.buffer.size(), c.id);

            c.headerReceived = 0;
            c.bytesReceived = 0;
        }
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

        Sleep(1);
    }

    WSACleanup();
    return 0;
}