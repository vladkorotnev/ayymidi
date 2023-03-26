#ifndef STATUS_H_
#define STATUS_H_
#include <stdint.h>

void status_regi_notify(uint8_t regi, uint8_t valu);
uint8_t status_regi_get_blocking(uint8_t regi);

#endif