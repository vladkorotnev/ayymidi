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

  midi_begin();
  disp_begin();
}


void loop() {
  midi_tick();
  disp_tick();
}
