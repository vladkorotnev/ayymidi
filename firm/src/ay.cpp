#include <ay.h>
#include <status.h>
#include <Arduino.h>


void ay_set_clock(){
  pinMode(3, OUTPUT);
  TCCR2A = 0x23;
  TCCR2B = 0x09;
  OCR2A = 8;
  OCR2B = 3; 
}

void ay_out_internal(unsigned char port, unsigned char data){
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

  status_regi_notify(port, data);
}

void ay_reset(){
  taskENTER_CRITICAL();

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

  taskEXIT_CRITICAL();
}

void ay_out(unsigned char port, unsigned char data){
  taskENTER_CRITICAL();

  ay_out_internal(port, data);

  taskEXIT_CRITICAL();
}