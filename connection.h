#pragma once

#include "thread.h"

class Connection
{
public:
    Connection();
    Connection(int socket);
    ~Connection();

    bool valid() const;

    int recv(char *data, unsigned int length) const;
    int send(const char *data, unsigned int length) const;
    int close();

private:
    class RxThread : public Thread
    {
    public:
        RxThread(Connection *connection);
        ~RxThread();

    private:
        void run() override;
        Connection *m_connection = nullptr;
    };

    class TxThread : public Thread
    {
    public:
        TxThread(Connection *connection);
        ~TxThread();

    private:
        void run() override;
        Connection *m_connection = nullptr;
    };

    int m_socket = -1;
    bool m_valid = false;
    RxThread m_rxThread;
    TxThread m_txThread;
};
