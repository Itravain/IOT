#include <Arduino.h>

typedef enum {IDLE, CLEAN, DOCK, CHRG, WAIT} Estado;

unsigned long timer;

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  pinMode(0, OUTPUT);
}

void loop() {
  static Estado estados = IDLE;
  static char command;
  switch (estados)
  {
  case IDLE:
      if(Serial.available()){
        if(Serial.read() == 'a'){
          estados = CLEAN;
          digitalWrite(2, 1);
          digitalWrite(0, 0);
          timer = millis();
        }
      }
      digitalWrite(2, 0);
      digitalWrite(0, 0);
    break;
  case CLEAN:
    if(millis() - timer >= 2000){
      estados = DOCK;
      digitalWrite(2, 0);
      digitalWrite(0, 1);
      timer = millis();
    }
    else if(Serial.available()){
      if(Serial.read() == 'b'){
        estados = DOCK;
        digitalWrite(2, 0);
        digitalWrite(0, 1);
        timer = millis();
      }
    }
    
    break;
    case DOCK:
    if(millis() - timer >= 2000){
        estados = CHRG;
        digitalWrite(2, 1);
        digitalWrite(0, 1);
        timer = millis();
      }
          
    break;
  case CHRG:
    if(millis() - timer >= 2000){
      estados = IDLE;
      digitalWrite(2, 0);
      digitalWrite(0, 1);
      timer = millis();
    }
    else if(Serial.available()){
      if(Serial.read() == 'a'){
        estados = CLEAN;
        digitalWrite(2, 0);
        digitalWrite(0, 1);
        timer = millis();
      }
    }
    
    break;
  
  default:
    break;
  }
}