#include <Arduino_FreeRTOS.h>
#include <ay.h>
#include <disp.h>
#include <midiif.h>

extern "C" {
  void setup();
  void loop();
}

void setup() {
  disp_begin();

  midi_begin();

  vTaskDelete(NULL); // Get rid of setup() and loop() task
}


void loop() {

}
