//Código para Arduino

#include "Arduino.h"

#define IDLE 0
#define CLEAN 1
#define DOCK 2
#define CHRG 3

#define LED1_PIN LED_BUILTIN_RX
#define LED2_PIN LED_BUILTIN_TX
#define DATAPATH1_PIN 6
#define DATAPATH2_PIN 8

int estadoAtual = IDLE;
unsigned long timerEstado;
unsigned long timerLED1;
unsigned long timerLED2;
boolean estadoLED1 = false;
boolean estadoLED2 = false;

#define TEMPO_TRANSICAO 5000
#define INTERVALO_LED1 100
#define INTERVALO_LED2 50


char lerComando() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    delay(10);
    return cmd;
  }
  return 0;
}

void mudarEstado(int novoEstado) {
  estadoAtual = novoEstado;
  timerEstado = millis();
  
  Serial.print("Estado alterado para: ");
  if (novoEstado == IDLE) Serial.println("IDLE");
  else if (novoEstado == CLEAN) Serial.println("CLEAN");
  else if (novoEstado == DOCK) Serial.println("DOCK");
  else if (novoEstado == CHRG) Serial.println("CHRG");
}

void idle() {
  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);

}

void cleaning() {
  digitalWrite(LED2_PIN, HIGH);
  
  if (millis() - timerLED1 >= INTERVALO_LED1) {
    estadoLED1 = !estadoLED1;
    digitalWrite(LED1_PIN, estadoLED1);
    timerLED1 = millis();
  }
}

void docking() {
  digitalWrite(LED1_PIN, HIGH);
  
  if (millis() - timerLED2 >= INTERVALO_LED2) {
    estadoLED2 = !estadoLED2;
    digitalWrite(LED2_PIN, estadoLED2);
    timerLED2 = millis();
  }
}

void charging() {
  if (millis() - timerLED1 >= INTERVALO_LED1) {
    estadoLED1 = !estadoLED1;
    digitalWrite(LED1_PIN, estadoLED1);
    timerLED1 = millis();
  }
  
  if (millis() - timerLED2 >= INTERVALO_LED2) {
    estadoLED2 = !estadoLED2;
    digitalWrite(LED2_PIN, estadoLED2);
    timerLED2 = millis();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(DATAPATH1_PIN, INPUT);
  pinMode(DATAPATH2_PIN, INPUT);
  
  timerEstado = millis();
  timerLED1 = millis();
  timerLED2 = millis();
  
  Serial.println("Roomba Inicializado!");
  Serial.println("Comandos: 'a' = Clean, 'b' = Dock");
  Serial.println("Estado atual: IDLE");
}

void loop() {
  char comando = lerComando();
  

      if (!digitalRead(DATAPATH1_PIN) && digitalRead(DATAPATH2_PIN)) {
        cleaning();
      }
      
      if (digitalRead(DATAPATH1_PIN) && !digitalRead(DATAPATH2_PIN)) {
        docking();
      }
     
      if (digitalRead(DATAPATH1_PIN) && digitalRead(DATAPATH2_PIN)) {
        charging();
      }
      if(!digitalRead(DATAPATH1_PIN) && !digitalRead(DATAPATH2_PIN)) {
        idle();
      }
}
