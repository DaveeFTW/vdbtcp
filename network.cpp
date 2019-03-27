#include "network.h"
#include "log.h"

#include <psp2/net/netctl.h>


Network::Network()
    : m_eventFlag("vdbtcp-net-ev")
{
    auto res = sceNetCtlInit();

    if (res < 0 && static_cast<unsigned int>(res) != 0x80412102)
    {
        LOG("sceNetCtlInit failed: 0x%08X\n", res);
        return;
    }

    res = sceNetCtlInetRegisterCallback(&networkEvent, this, &m_cid);

    if (res < 0)
    {
        LOG("Network init failed: sceNetCtlInetRegisterCallback 0x%08X\n", res);
        return;
    }

    m_eventFlag.clear(CONNECTED);
    m_connected = false;

    m_running = true;
    start();
}

Network::~Network()
{
    m_running = false;
}

bool Network::connected() const
{
    return m_connected;
}

void Network::waitUntilConnected()
{
    auto res = m_eventFlag.waitFor(CONNECTED, false);

    if (res < 0)
    {
        LOG("error waiting for network event flag: 0x%08X\n", res);
    }
}

void *Network::onEvent(SceNetCtlState eventType)
{
    if (eventType == SCE_NETCTL_STATE_CONNECTED)
    {
        m_eventFlag.set(CONNECTED);
        m_connected = true;
    }
    else
    {
        m_eventFlag.clear(CONNECTED);
        m_connected = false;
    }

    return NULL;
}

void Network::run()
{
    while (m_running)
    {
        sceNetCtlCheckCallback();
        sceKernelDelayThread(10000);
    }
}

void *Network::networkEvent(int eventType, void *arg)
{
    Network *network = reinterpret_cast<Network *>(arg);
    return network->onEvent(static_cast<SceNetCtlState>(eventType));
}
