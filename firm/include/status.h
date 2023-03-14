#ifndef STATUS_H_
#define STATUS_H_

#include <Arduino_FreeRTOS.h>

typedef uint32_t regi_valu_tuple_t;
#define REGI_OF_TUPLE(x) (x & 0xF)
#define VALU_OF_TUPLE(x) ((x >> 8) & 0xFF)

regi_valu_tuple_t status_wait_change_regi();
void status_regi_notify(uint8_t regi, uint8_t valu);
uint8_t status_regi_get_blocking(uint8_t regi);

#endif