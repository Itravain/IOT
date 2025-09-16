#include "Arduino.h"

void setup(){
    pinMode(LED_BUILTIN_TX, OUTPUT);
    pinMode(LED_BUILTIN_RX, OUTPUT);
}


void loop(){
    digitalWrite(LED_BUILTIN_TX, 1);
    digitalWrite(LED_BUILTIN_RX, 1);
    delay(1000);
    digitalWrite(LED_BUILTIN_TX, 0);
    digitalWrite(LED_BUILTIN_RX, 0);
    delay(1000);


}