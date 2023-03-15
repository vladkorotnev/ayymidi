#include <Arduino_FreeRTOS.h>
#include <log.h>
#include <ay.h>
#include <disp.h>
#include <midiif.h>

extern "C" {
  void setup();
  void loop();
}

void setup() {
  log_begin();

  inf_log(F("Starting..."));

  disp_begin();

  midi_begin();

  vTaskStartScheduler();

  // this should not execute
  err_log(F("Why are we here??"));
  for(;;);
}


void loop() {

}
