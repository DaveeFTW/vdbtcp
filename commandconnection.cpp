#include "commandconnection.h"
#include "log.h"

#include <vdb.h>

#include <psp2/net/net.h>
#include <psp2/kernel/error.h>
#include <psp2/appmgr.h>
#include <psp2/power.h>

#include <stdio.h>
#include <string.h>

namespace 
{
    using Handler = void (*)(const char *argument, char *output, size_t outSize);

    void cmd_destroy(const char *argument, char *output, size_t outSize)
    {
        sceAppMgrDestroyOtherApp();
    }

    void cmd_launch(const char *argument, char *output, size_t outSize)
    {
        LOG("about to launch app\n");
        auto res = vdb_launch_debug(argument);

        if (res < 0)
        {
            snprintf(output, outSize, "Error 0x%08X launching \"%s\"\n", res, argument);
        }
    }

    void cmd_reboot(const char *argument, char *output, size_t outSize)
    {
        scePowerRequestColdReset();
    }

    Handler lookupCommandHandler(const char *name)
    {
        constexpr struct {
            const char *name;
            Handler executor;
        } handlers[] = 
        {
            { "destroy", cmd_destroy },
            { "launch", cmd_launch },
            { "reboot", cmd_reboot },
        };

        constexpr size_t handlerCount = sizeof(handlers)/sizeof(handlers[0]);

        for (auto i = 0u; i < handlerCount; ++i)
        {
            if (strcmp(handlers[i].name, name) == 0)
            {
                return handlers[i].executor;
            }
        }

        return nullptr;
    }
}

CommandConnection::CommandConnection(int socket)
    : m_socket(socket)
    , m_valid(true)
    , m_commandThread(this)
{
    LOG("we got this far COMMADN\n");
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

    m_commandThread.start();
}

CommandConnection::~CommandConnection()
{
    if (!m_valid)
    {
        return;
    }

    close();
}

bool CommandConnection::valid() const
{
    return m_valid;
}

int CommandConnection::recv(char *data, unsigned int length) const
{
    return sceNetRecv(m_socket, data, length, 0);
}

int CommandConnection::send(const char *data, unsigned int length) const
{
    return sceNetSend(m_socket, data, length, 0);
}

int CommandConnection::close()
{
    auto res = sceNetSocketClose(m_socket);
    m_socket = -1;
    m_valid = false;
    return res;
}

CommandConnection::CommandThread::CommandThread(CommandConnection *connection)
    : m_connection(connection)
{
    setStackSize(8*1024);
}

void CommandConnection::CommandThread::run()
{
    LOG("we got this far\n");
    constexpr auto maxBufferSize = 0x800u;
    char data[maxBufferSize+1];

    if (readCommand(data, maxBufferSize))
    {
        LOG("Got command \"%s\"\n", data);
        char output[maxBufferSize+1];
        *output = '\0';

        auto space = strchr(data, ' ');
        auto command = data;
        const char *argument = "";

        if (space)
        {
            *space = '\0';
            argument = space+1;
        }

        auto handler = lookupCommandHandler(command);

        if (handler)
        {
            LOG("Found command handler \"%s\"\n", command);
            handler(argument, output, maxBufferSize);
        }
        else
        {
            snprintf(output, maxBufferSize, "Error unrecognised command \"%s\"\n", command);
        }

        LOG("Command output \"%s\"\n", output);
        m_connection->send(output, strlen(output));
    }
    else
    {
        LOG("Error reading command\n");
    }
    

    m_connection->close();
}

bool CommandConnection::CommandThread::readCommand(char *output, size_t size) const
{
    size_t endPosition = 0u;

    while (m_connection->valid() && endPosition < size)
    {
        auto res = m_connection->recv(output+endPosition, size-endPosition);
        LOG("Read 0x%08X from connection\n", res);

        if (res < 0)
        {
            LOG("error receiving data from TCP: 0x%08X\n", res);
            break;
        }

        else if (res == 0)
        {
            LOG("NO MORE DATA\n");
            break;
        }

        for (auto i = 0; i < res; ++i)
        {
            if (output[i+endPosition] == '\n')
            {
                output[i+endPosition] = '\0';
                return true;
            }
        }

        endPosition += res;
    }

    return false;
}
