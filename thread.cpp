#include "thread.h"
#include "log.h"

#include <psp2/kernel/threadmgr.h>

Thread::Thread(const char *name)
    : m_name(name)
{

}

int Thread::start()
{
    int res = sceKernelCreateThread(m_name, &threadEntry, 0x40, m_stackSize, 0, 0, NULL);

    if (res < 0)
    {
        LOG("failed to create thread: 0x%08X\n", res);
        return res;
    }

    Thread *ptr = this;
    res = sceKernelStartThread(res, sizeof(ptr), &ptr);

    if (res < 0)
    {
        LOG("failed to start thread 0x%08X\n", res);
        return res;
    }

    return 0;
}

unsigned int Thread::stackSize() const
{
    return m_stackSize;
}

void Thread::setStackSize(unsigned int size)
{
    m_stackSize = size;
}

int Thread::threadEntry(SceSize args, void *argp)
{
    Thread *ptr = *reinterpret_cast<Thread **>(argp);
    ptr->run();
    return 0;
}
