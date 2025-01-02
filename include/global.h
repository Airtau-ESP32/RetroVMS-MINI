// File: global.h
#ifndef GLOBAL_H
#define GLOBAL_H

#pragma once

#include <stdint.h>

extern uint32_t monotonictime;

#ifdef CONFIG_FREERTOS_UNICORE
  #define CORE(n) (0)
#else
  #define CORE(n) (n)
#endif

extern const char *appName;
extern const char *appVersion;

#endif // GLOBAL_H