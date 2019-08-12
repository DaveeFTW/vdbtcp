#include "connection.h"
#include "log.h"

#include <vdb.h>

#include <psp2/net/net.h>
#include <psp2/kernel/error.h>

// TODO: add to SDK
#include <psp2/kernel/threadmgr.h>
extern "C" int sceKernelOpenMsgPipe(const char *name);

Connection::Connection()
    : m_rxThread(this)
    , m_txThread(this)
{
}

Connection::Connection(int socket)
    : m_socket(socket)
    , m_valid(true)
    , m_rxThread(this)
    , m_txThread(this)
{
    auto nonblocking = 0;
    auto res = sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &nonblocking, sizeof(nonblocking));

    if (res < 0)
    {
        LOG("error setting socket to blocking I/O (0x%08X)\n", res);
        return;
    }

    auto set = 1;
    res = sceNetSetsockopt(socket, SCE_NET_SOL_SOCKET, SCE_NET_TCP_NODELAY, &set, sizeof(set));

    if (res < 0)
    {
        LOG("error setting TCP no-delay on socket (0x%08X)\n", res);
    }

    // start our threads
    m_rxThread.start();
    m_txThread.start();
}

Connection::~Connection()
{
    if (!m_valid)
    {
        return;
    }

    close();
}

bool Connection::valid() const
{
    return m_valid;
}

int Connection::recv(char *data, unsigned int length) const
{
    return sceNetRecv(m_socket, data, length, 0);
}

int Connection::send(const char *data, unsigned int length) const
{
    return sceNetSend(m_socket, data, length, 0);
}

int Connection::close()
{
    auto res = sceNetSocketClose(m_socket);
    m_socket = -1;
    m_valid = false;
    return res;
}

Connection::RxThread::RxThread(Connection *connection)
    : Thread("tcp-rx-thread")
    , m_connection(connection)
{
    setStackSize(0x3000);
}

Connection::RxThread::~RxThread()
{
}

void Connection::RxThread::run()
{
    while (m_connection->valid())
    {
        char data[0x2000];
        auto res = m_connection->recv(data, sizeof(data));

        if (res < 0)
        {
            LOG("error receiving data from TCP: 0x%08X\n", res);
            break;
        }

        else if (res == 0)
        {
            break;
        }

        res = vdb_send_serial_pipe(data, res);

        if (res < 0)
        {
            LOG("error sending data through pipe: 0x%08X\n", res);
            break;
        }
    }

    m_connection->close();
}

Connection::TxThread::TxThread(Connection *connection)
    : Thread("tcp-tx-thread")
    , m_connection(connection)
{
    setStackSize(0x3000);
}

Connection::TxThread::~TxThread()
{
}

void Connection::TxThread::run()
{
    while (m_connection->valid())
    {
        unsigned int size = 0;
        char data[0x2000];

        auto timeout = 100000u; // us
        // receive data from the kernel
        auto res = vdb_recv_serial_pipe(data, sizeof(data), timeout);

        if (res < 0)
        {
            if (static_cast<unsigned int>(res) == SCE_KERNEL_ERROR_WAIT_TIMEOUT)
            {
                continue;
            }

            LOG("error receiving data from the kernel 0x%08X\n", res);
            return;
        }

        size = res;
        res = m_connection->send(data, size);

        if (res < 0)
        {
            LOG("error sending data over TCP 0x%08X\n", res);
            break;
        }
    }

    m_connection->close();
}
