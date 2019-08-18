#pragma once

#include "thread.h"
#include "network.h"
#include "log.h"

#include <psp2/kernel/threadmgr.h>

template <typename TConnection>
class Server : private Thread
{
public:
    Server();

    bool listen(int port);

private:
    int m_socket = -1;

    void onNewConnection(int socket);
    void run() override;
};

template <typename TConnection>
Server<TConnection>::Server()
{
    m_socket = sceNetSocket("vdbtcp-server", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    if (m_socket < 0)
    {
        LOG("failed to create server socket: 0x%08X\n", m_socket);
        return;
    }

    auto nonblocking = 1;
    auto res = sceNetSetsockopt(m_socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &nonblocking, sizeof(nonblocking));

    if (res < 0)
    {
        LOG("failed to mark port as non-blocking I/O: 0x%08X\n", res);
        return;
    }
}

template <typename TConnection>
bool Server<TConnection>::listen(int port)
{
    SceNetSockaddrIn addr = { 0 };

    addr.sin_family = SCE_NET_AF_INET;
    addr.sin_port = sceNetHtons(port);
    addr.sin_addr.s_addr = 0;

    auto res = sceNetBind(m_socket, (const SceNetSockaddr *)&addr, sizeof(addr));

    if (res < 0)
    {
        LOG("failed to bind to port %d: 0x%08X\n", port, res);
        return false;
    }

    res = sceNetListen(m_socket, 1);

    if (res < 0)
    {
        LOG("failed to listen on port %d: 0x%08X\n", port, res);
        return false;
    }

    if (start() < 0)
    {
        LOG("could not start thread for listening\n");
        return false;
    }

    return true;
}

template <typename TConnection>
void Server<TConnection>::run()
{
    while (1)
    {
        SceNetSockaddr addr;
        unsigned int len = sizeof(addr);

        auto res = sceNetAccept(m_socket, &addr, &len);

        if (res < 0)
        {
            if (static_cast<unsigned>(res) != SCE_NET_ERROR_EAGAIN
                && static_cast<unsigned>(res) != 0x804101A3)
            {
                // an actual error occured
                break;
            }
        }
        else
        {
            onNewConnection(res);
        }

        // yield
        sceKernelDelayThread(100);
    }
}

template <typename TConnection>
void Server<TConnection>::onNewConnection(int socket)
{
    TConnection connection(socket);

    while (connection.valid())
    {
        // yield
        sceKernelDelayThread(100);
    }
}
