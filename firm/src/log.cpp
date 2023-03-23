#include <log.h>
#include <Arduino.h>
#include <stdarg.h>

#if !defined(USE_SOFTSERIAL) && defined(SERIAL_LOG)
#error Logger will take over UART. Please use SoftSerial or disable logging.
#endif

#if defined(SERIAL_LOG)
void log_begin() {
    Serial.begin(9600);
    inf_log(F("*Ashens voice*: Hello!"));
}

void do_logging(const char * prefix, const __FlashStringHelper * fmt, va_list ap) {
    char buf[64];
    vsnprintf_P(buf, sizeof(buf), (const char *)fmt, ap);
    for(const char *p = &prefix[0]; *p; p++)
            Serial.write(*p);
    for(char *p = &buf[0]; *p; p++)
            Serial.write(*p);
    Serial.write('\r');
    Serial.write('\n');
}

void dbg_log(const __FlashStringHelper * fmt, ...) {
#ifdef DEBUG_LOG
    va_list ap;
    va_start(ap, fmt);
    do_logging("[DBG] ", fmt, ap);
    va_end(ap);
#endif
}

void inf_log(const __FlashStringHelper * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    do_logging("[INF] ", fmt, ap);
    va_end(ap);
}

void err_log(const __FlashStringHelper * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    do_logging("[ERR] ", fmt, ap);
    va_end(ap);
}
#else
void log_begin() {}
void dbg_log(const __FlashStringHelper *, ...) {}
void inf_log(const __FlashStringHelper *, ...) {}
void err_log(const __FlashStringHelper *, ...) {}
#endif