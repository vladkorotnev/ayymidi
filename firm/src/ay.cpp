#include <ay.h>
#include <status.h>
#include <util.h>
#include <Arduino.h>
#ifdef HAVE_FASTPWMPIN
#include <FastPwmPin.h>
#endif

#define USE_TIMER_2 false
#define USE_TIMER_1 true
#include <TimerInterrupt.h>

volatile bool sidfx_running = false;
volatile bool sidfx_phase = true;
volatile uint8_t sidfx_chan = 0;
uint32_t ay_clk = 0;

//
// Based upon the code samples from https://habr.com/ru/post/392625/
//

void ay_set_clock(uint32_t clock){
  ay_clk = clock;
  #ifdef HAVE_FASTPWMPIN
    FastPwmPin::enablePwmPin(3, clock, FASTPWMPIN_TOGGLE);
  #else
    // Legacy code for 1.777 MHz
    err_log(F("Build without variable clock support!"));
    pinMode(3, OUTPUT);
    TCCR2A = 0x23;
    TCCR2B = 0x09;
    OCR2A = 8;
    OCR2B = 3; 
  #endif
}

void ay_out_internal(uint8_t port, uint8_t data){
  PORTB = PORTB & B11111100;

  PORTC = port & B00001111;
  PORTD = PORTD & B00001111;

  PORTB = PORTB | B00000011;
  delayMicroseconds(1);
  PORTB = PORTB & B11111100;

  PORTC = data & B00001111;
  PORTD = (PORTD & B00001111) | (data & B11110000);

  PORTB = PORTB | B00000010;
  delayMicroseconds(1);
  PORTB = PORTB & B11111100;
}

void ay_out(uint8_t port, uint8_t data){
  if(!sidfx_running || port != (AY_REGI_LVL_A + sidfx_chan)) {
    ay_out_internal(port, data);
  }
  status_regi_notify(port, data);
}

void sidfx_timer_handler(void) {
  cli();
  uint8_t tgt_regi = AY_REGI_TONE_A_FINE + sidfx_chan*2;
  ay_out_internal(tgt_regi, 0);
  ay_out_internal(tgt_regi, status_regi_get_blocking(tgt_regi));
  sei();
}

inline void sidfx_start_internal(uint8_t chan, uint32_t freq) {
  float f = freq * 1.8;
  sidfx_running = true;
  sidfx_chan = chan;
  ITimer1.attachInterrupt(f, sidfx_timer_handler);
}

void sidfx_stop() {
  ITimer1.disableTimer();
}

void sidfx_start(uint8_t chan) {
  uint16_t tone = (status_regi_get_blocking(AY_REGI_TONE_A_COARSE + (chan * 2)) << 8) | status_regi_get_blocking(AY_REGI_TONE_A_FINE + (chan * 2));
  uint32_t tone_f = ay_clk / (16 * tone);
  sidfx_start_internal(chan, tone_f);
}

void ay_reset(){
  ITimer1.init();

  pinMode(A0, OUTPUT); // D0
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT); // D3
  
  pinMode(4, OUTPUT); // D4
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT); // D7
  
  pinMode(8, OUTPUT);  // BC1
  pinMode(9, OUTPUT);  // BDIR
  
  digitalWrite(8,LOW);
  digitalWrite(9,LOW);
  
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delayMicroseconds(100);
  digitalWrite(2, HIGH);
  delayMicroseconds(100);

  for (int i=0;i<16;i++) ay_out_internal(i,0);
}