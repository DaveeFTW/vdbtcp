#include "thread.h"
#include "log.h"

#include <psp2/kernel/threadmgr.h>

Thread::Thread(const char *name)
    : m_name(name)
{

}

Thread::~Thread()
{
    sceKernelWaitThreadEnd(m_thid, nullptr, nullptr);
    sceKernelDeleteThread(m_thid);
}

int Thread::start()
{
    const auto thid = sceKernelCreateThread(m_name, &threadEntry, 0x40, m_stackSize, 0, 0, NULL);

    if (thid < 0)
    {
        LOG("failed to create thread: 0x%08X\n", thid);
        return thid;
    }

    Thread *ptr = this;
    const auto res = sceKernelStartThread(thid, sizeof(ptr), &ptr);

    if (res < 0)
    {
        LOG("failed to start thread 0x%08X\n", res);
        return res;
    }

    m_thid = thid;
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
