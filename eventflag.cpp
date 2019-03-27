#include "eventflag.h"
#include "log.h"

#include <psp2/kernel/threadmgr.h>

EventFlag::EventFlag(const char *name)
{
    m_evid = sceKernelCreateEventFlag(name, 0, 0, NULL);

    if (m_evid < 0)
    {
        LOG("error creating event flag \"%s\": 0x%08X\n", name, m_evid);
    }
}

EventFlag::~EventFlag()
{
    sceKernelDeleteEventFlag(m_evid);
}

int EventFlag::waitFor(unsigned int value, unsigned int *pattern)
{
    return waitFor(value, true, pattern);
}

int EventFlag::waitFor(unsigned int value, bool clear, unsigned int *pattern)
{
    unsigned int flags = SCE_EVENT_WAITAND;

    if (clear)
    {
        flags |= SCE_EVENT_WAITCLEAR;
    }

    unsigned int patternInt = 0;
    auto res = sceKernelWaitEventFlag(m_evid, value, flags, &patternInt, NULL);

    if (res < 0)
    {
        LOG("sceKernelWaitEventFlag failed flg: 0x%08X, bits 0x%08X, flags: 0x%08X (0x%08X)\n", m_evid, value, flags, res);
        return res;
    }

    if (pattern)
    {
        *pattern = patternInt;
    }

    return 0;
}

int EventFlag::waitForAny(unsigned int value, unsigned int *pattern)
{
    unsigned int patternInt = 0;
    auto res = sceKernelWaitEventFlag(m_evid, value, SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR, &patternInt, NULL);

    if (res < 0)
    {
        LOG("sceKernelWaitEventFlag failed (0x%08X)\n", res);
        return res;
    }

    if (pattern)
    {
        *pattern = patternInt;
    }

    return 0;
}

int EventFlag::set(unsigned int value)
{
    return sceKernelSetEventFlag(m_evid, value);
}

int EventFlag::clear(unsigned int value)
{
    return sceKernelClearEventFlag(m_evid, value);
}
