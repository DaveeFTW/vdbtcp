#include "connection.h"
#include "network.h"
#include "server.h"
#include "log.h"

#include <vdb.h>

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/io/fcntl.h>

namespace
{
    bool removeSelf()
    {
        int res = sceIoRemove("ux0:tai/vdbtcp.suprx");

        if (res < 0)
        {
            LOG("failed to remove self: 0x%08X\n", res);
            return false;
        }

        return true;
    }

    int threadEntry(SceSize args, void *argp)
    {
        if (!removeSelf())
        {
            return 1;
        }

        // wait 5 seconds for VSH to load
        sceKernelDelayThread(5*1000*1000);

        // setup networking
        Network network;

        while (1)
        {
            network.waitUntilConnected();

            Server server;

            if (!server.listen(31337))
            {
                if (!network.connected())
                {
                    LOG("network disconnected: retrying connection\n");
                    continue;
                }
                else
                {
                    LOG("failed to listen to port. aborting\n");
                    break;
                }
            }

            LOG("server listening\n");

            while (network.connected())
            {
                auto connection = server.waitForConnection();

                LOG("got connection\n");

                while (connection.valid())
                {
                    // yield
                    sceKernelDelayThread(100);
                }
            }
        }

        return 0;
    }
}

extern "C"
{
int module_start()
{
    LOG("begin\n");

    // tell vdb we want to use its pipes. we will transmit our data from
    // tcp through these pipes
    vdb_serial_pipe();

    int res = sceKernelCreateThread("vdbtcp", &threadEntry, 0x40, 0x1000, 0, 0, NULL);

    if (res < 0)
    {
        LOG("failed to create main thread: 0x%08X\n", res);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    res = sceKernelStartThread(res, 0, NULL);

    if (res < 0)
    {
        LOG("failed to start main thread 0x%08X\n", res);
        return SCE_KERNEL_START_NO_RESIDENT;
    }

    return SCE_KERNEL_START_RESIDENT;
}

int module_stop()
{
    return 0;
}

int _start() __attribute__ ((weak, alias ("module_start")));
}