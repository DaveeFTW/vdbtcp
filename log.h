#ifndef LOG_H
#define LOG_H

#include <psp2/kernel/clib.h>

#define LOG(fmt, ...) sceClibPrintf("[vdbtcp] " fmt, ##__VA_ARGS__)

#endif // LOG_H
