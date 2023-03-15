#include <status.h>
#include <semphr.h>

static volatile uint8_t register_dump[0x10] = { 0 };
static volatile TaskHandle_t subscriber_task = nullptr;

static SemaphoreHandle_t dump_tbl_semaphore = nullptr;

#define TUPLE_OF(r, v) (regi_valu_tuple_t)((r & 0xF) | ((v & 0xFF) << 8))

inline void _setup_semaphore_if_needed() {
    if(dump_tbl_semaphore == nullptr) {
        dump_tbl_semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(dump_tbl_semaphore);
    }
}

regi_valu_tuple_t status_wait_change_regi() {
    uint32_t regi_valu_tuple = 0x0;
    subscriber_task = xTaskGetCurrentTaskHandle();
    xTaskNotifyWait(0x0, 0xFF, &regi_valu_tuple, portMAX_DELAY);
    return regi_valu_tuple;
}

void status_regi_notify(uint8_t regi, uint8_t valu) {
    if(regi > 0xF) return;
    _setup_semaphore_if_needed();

    if(xSemaphoreTake(dump_tbl_semaphore, pdMS_TO_TICKS(1)) == pdTRUE) {
        register_dump[regi] = valu;
        if(subscriber_task != nullptr) {
            xTaskNotify(subscriber_task, TUPLE_OF(regi, valu), eNotifyAction::eSetValueWithoutOverwrite);
            subscriber_task = nullptr;
        }
        xSemaphoreGive(dump_tbl_semaphore);
    }
}

uint8_t status_regi_get_blocking(uint8_t regi) {
    _setup_semaphore_if_needed();
    uint8_t val = 0;
    if(xSemaphoreTake(dump_tbl_semaphore, portMAX_DELAY) == pdTRUE) {
        val = register_dump[regi];
        xSemaphoreGive(dump_tbl_semaphore);
    }
    return val;
}