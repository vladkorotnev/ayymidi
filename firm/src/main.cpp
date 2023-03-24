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

  // while(!Serial.available());

  // byte b = 0x0;
  // while (b != 0xF0)
  // {
  //   while(!Serial.available());
  //   b = Serial.read();
  // }
  
  //     Serial.print(b, HEX);
  // while(b != 0xF7) {
  //   while(!Serial.available());
  //   b = Serial.read();
  //     if (b < 16) {Serial.print("0");}
  //     Serial.print(b, HEX);
  //     Serial.print(" ");
  // }
  // Serial.println("");
}
