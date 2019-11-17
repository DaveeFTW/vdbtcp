#pragma once

#include "thread.h"

class CommandConnection
{
public:
    CommandConnection(int socket);
    ~CommandConnection();

    bool valid() const;

    int recv(char *data, unsigned int length) const;
    int send(const char *data, unsigned int length) const;
    int close();

private:
    class CommandThread : public Thread
    {
    public:
        CommandThread(CommandConnection *connection);

    private:
        void run() override;
        bool readCommand(char *output, size_t size) const;

        CommandConnection *m_connection = nullptr;
    };

    int m_socket = -1;
    bool m_valid = false;
    CommandThread m_commandThread;
};
