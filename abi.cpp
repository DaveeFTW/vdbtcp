#include "log.h"
#include <psp2/kernel/threadmgr.h>

extern "C"
{
void __cxa_pure_virtual()
{
    LOG("pure virtual called... sleeping forever.");

    while (1)
    {
        sceKernelDelayThread(1000);
    }
}
}
