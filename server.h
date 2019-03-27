#pragma once

#include "thread.h"
#include "eventflag.h"
#include "connection.h"

class Network;

class Server : private Thread
{
public:
    Server();

    bool listen(int port);
    Connection waitForConnection();

private:
    int m_socket = -1;

    void onNewConnection(int socket);
    void run() override;

    enum Flag
    {
        CONNECTION = (1 << 31),
    };

    EventFlag m_eventFlag;
};
