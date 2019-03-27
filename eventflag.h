#pragma once

#include <psp2/kernel/threadmgr.h>

class EventFlag
{
public:
    EventFlag(const char *name = "");
    ~EventFlag();

    int waitFor(unsigned int value, unsigned int *pattern = nullptr);
    int waitFor(unsigned int value, bool clear = true, unsigned int *pattern = nullptr);
    int waitForAny(unsigned int value, unsigned int *pattern = nullptr);
    int set(unsigned int value);
    int clear(unsigned int value);

private:
    int m_evid = -1;
};
