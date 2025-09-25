#include "Arduino.h"

#define IDLE 0
#define CLEAN 1
#define DOCK 2
#define CHRG 3

#define LED1_PIN 2
#define LED2_PIN 4
#define BATERY_LED 26
#define DATAPATH1_PIN 13
#define DATAPATH2_PIN 12

#define DIST_PIN0 32
#define DIST_PIN1 33
#define LUZ_PIN   34
#define UMID_PIN  35

int estadoAtual = IDLE;
unsigned long timerEstado;
unsigned long timerLED1;
unsigned long timerLED2;
unsigned long timerPrint;

unsigned long duracao;
boolean estadoLED1 = false;
boolean estadoLED2 = false;

#define TEMPO_TRANSICAO 5000
#define INTERVALO_LED1 100
#define INTERVALO_LED2 50
#define INTERVALO_PRINT 1000

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
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(DATAPATH1_PIN, LOW);
  digitalWrite(DATAPATH2_PIN, LOW);
}

void cleaning() {
  dacWrite(BATERY_LED, 255 - duracao / (float)TEMPO_TRANSICAO * 255);
  digitalWrite(DATAPATH1_PIN, HIGH);
  digitalWrite(DATAPATH2_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  if (millis() - timerLED1 >= INTERVALO_LED1) {
    estadoLED1 = !estadoLED1;
    digitalWrite(LED1_PIN, estadoLED1);
    timerLED1 = millis();
  }
}

void docking() {
  digitalWrite(DATAPATH1_PIN, LOW);
  digitalWrite(DATAPATH2_PIN, HIGH);
  digitalWrite(LED1_PIN, LOW);

  if (millis() - timerLED2 >= INTERVALO_LED2) {
    estadoLED2 = !estadoLED2;
    digitalWrite(LED2_PIN, estadoLED2);
    timerLED2 = millis();
  }
}

void charging() {
  dacWrite(BATERY_LED, duracao / (float)TEMPO_TRANSICAO * 255);
  digitalWrite(DATAPATH1_PIN, HIGH);
  digitalWrite(DATAPATH2_PIN, HIGH);

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
  pinMode(DATAPATH1_PIN, OUTPUT);
  pinMode(DATAPATH2_PIN, OUTPUT);

  pinMode(DIST_PIN0, INPUT);
  pinMode(DIST_PIN1, INPUT);
  pinMode(LUZ_PIN, INPUT);
  pinMode(UMID_PIN, INPUT);

  timerEstado = millis();
  timerLED1 = millis();
  timerLED2 = millis();
  timerPrint = millis();

  Serial.println("Roomba Inicializado!");
  Serial.println("Comandos: 'a' = Clean, 'b' = Dock");
  Serial.println("Estado atual: IDLE");
}

void loop() {
  char comando = lerComando();
  duracao = millis() - timerEstado;

  switch (estadoAtual) {
    case IDLE:
      idle();
      if (comando == 'a') mudarEstado(CLEAN);
      break;
    case CLEAN:
      cleaning();
      if (comando == 'b') mudarEstado(DOCK);
      else if (duracao >= TEMPO_TRANSICAO) mudarEstado(DOCK);
      break;
    case DOCK:
      docking();
      if (duracao >= TEMPO_TRANSICAO) mudarEstado(CHRG);
      break;
    case CHRG:
      charging();
      if (comando == 'a') mudarEstado(CLEAN);
      else if (duracao >= TEMPO_TRANSICAO) mudarEstado(IDLE);
      break;
    default:
      mudarEstado(IDLE);
      break;
  }

  if (millis() - timerPrint >= INTERVALO_PRINT) {
    int distBit0 = digitalRead(DIST_PIN0);
    int distBit1 = digitalRead(DIST_PIN1);
    int estadoDist = (distBit1 << 1) | distBit0;

    int estadoLuz = digitalRead(LUZ_PIN);
    int estadoUmid = digitalRead(UMID_PIN);

    Serial.println("---------- STATUS ESP32 ----------");
    Serial.print("Estado do robô: ");
    if (estadoAtual == IDLE) Serial.println("IDLE");
    else if (estadoAtual == CLEAN) Serial.println("CLEAN");
    else if (estadoAtual == DOCK) Serial.println("DOCK");
    else if (estadoAtual == CHRG) Serial.println("CHARGING");

    Serial.print("Distância: ");
    if (estadoDist == 0) Serial.println("Sem obstáculo");
    else if (estadoDist == 1) Serial.println("Obstáculo longe");
    else if (estadoDist == 2) Serial.println("Obstáculo meia distância");
    else Serial.println("Obstáculo perto");

    Serial.print("Luminosidade: ");
    if (estadoLuz == 0) Serial.println("Sem obstáculo acima");
    else Serial.println("Obstáculo acima");

    Serial.print("Umidade: ");
    if (estadoUmid == 0) Serial.println("Aceitável");
    else Serial.println("Não aceitável");

    Serial.println("----------------------------------");
    timerPrint = millis();
  }
}
