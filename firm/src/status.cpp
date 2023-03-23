#include <status.h>
#include <util.h>

static uint8_t register_dump[0x10] = { 0 };

void status_regi_notify(uint8_t regi, uint8_t valu) {
    if(regi > 0xF) return;
    register_dump[regi] = valu;
}

uint8_t status_regi_get_blocking(uint8_t regi) {
    uint8_t val =  register_dump[regi];
    return val;
}