#include "ftpconnection.h"
#include "ftp.h"
#include "log.h"

#include <psp2/net/net.h>
#include <psp2/kernel/error.h>

FtpConnection::FtpConnection(int socket)
    : m_socket(socket)
    , m_valid(true)
    , m_ftpThread(this)
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

    m_ftpThread.start();
}

FtpConnection::~FtpConnection()
{
    if (!m_valid)
    {
        return;
    }

    close();
}

bool FtpConnection::valid() const
{
    return m_valid;
}

int FtpConnection::close()
{
    auto res = sceNetSocketClose(m_socket);
    m_socket = -1;
    m_valid = false;
    return res;
}

FtpConnection::FtpThread::FtpThread(FtpConnection *connection)
    : m_connection(connection)
{
    setStackSize(30*1024);
}

void FtpConnection::FtpThread::run()
{
    do_ftp(m_connection->m_socket);
    m_connection->close();
}
