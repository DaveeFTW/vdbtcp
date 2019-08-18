#pragma once

#include "thread.h"

class FtpConnection
{
public:
    FtpConnection(int socket);
    ~FtpConnection();

    bool valid() const;
    int close();

private:
    class FtpThread : public Thread
    {
    public:
        FtpThread(FtpConnection *connection);

    private:
        void run() override;
        FtpConnection *m_connection = nullptr;
    };

    int m_socket = -1;
    bool m_valid = false;
    FtpThread m_ftpThread;
};
