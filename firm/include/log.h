#ifndef LOG_H_
#define LOG_H_
#include <stdarg.h>
#include <Arduino.h>

#define LOG_BAUD 9600
void log_begin();
void dbg_log(const __FlashStringHelper *, ...);
void inf_log(const __FlashStringHelper *, ...);
void err_log(const __FlashStringHelper *, ...);

#endif