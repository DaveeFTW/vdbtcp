#pragma once

#include "eventflag.h"
#include "thread.h"

#include <psp2/net/netctl.h>
#include <atomic>

class Network : private Thread
{
public:
    Network();
    ~Network();

    bool connected() const;
    void waitUntilConnected();

private:
    int m_cid = -1;
    bool m_connected = false;
    EventFlag m_eventFlag;
    std::atomic<bool> m_running { false };

    enum Status
    {
        CONNECTED = (1 << 0)
    };

    void *onEvent(SceNetCtlState eventType);
    void run() override;
    static void *networkEvent(int eventType, void *arg);
};
