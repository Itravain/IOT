#include "Arduino.h"
#include <NewPing.h>
#include <DHT.h>

#define IDLE 0
#define CLEAN 1
#define DOCK 2
#define CHRG 3

#define LED1_PIN LED_BUILTIN_RX
#define LED2_PIN LED_BUILTIN_TX
#define DATAPATH1_PIN 6
#define DATAPATH2_PIN 8

#define HC_TRIGGER_PIN 7
#define HC_ECHO_PIN 9
#define HC_MAX_DISTANCE 400
NewPing sonar(HC_TRIGGER_PIN, HC_ECHO_PIN, HC_MAX_DISTANCE);

#define DHT_DATA 3
#define DHTTYPE DHT11
DHT dht(DHT_DATA, DHTTYPE);

#define LDR_PIN A0

#define DIST_PIN0 15
#define DIST_PIN1 14
#define LUZ_PIN   16
#define UMID_PIN  10

unsigned long timerEstado;
unsigned long timerLED1;
unsigned long timerLED2;
unsigned long timerPrint;

boolean estadoLED1 = false;
boolean estadoLED2 = false;

#define TEMPO_TRANSICAO 5000
#define INTERVALO_LED1 100
#define INTERVALO_LED2 50
#define INTERVALO_PRINT 1000

int estadoAtual = IDLE;

int estadoDist = 0;
int ultimoEstadoDist = -1;

int estadoLuz = 0;
int ultimoEstadoLuz = -1;

int estadoUmid = 0;
int ultimoEstadoUmid = -1;

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

void lerSensores() {
  unsigned int dist = sonar.ping_cm();
  if (dist == 0) dist = HC_MAX_DISTANCE;
  if (dist > 50) estadoDist = 0;
  else if (dist > 20) estadoDist = 1;
  else if (dist > 10) estadoDist = 2;
  else estadoDist = 3;

  if (estadoDist != ultimoEstadoDist) {
    // Serial.print("Distância mudou para: ");
    // if (estadoDist == 0) Serial.println("Sem obstáculo");
    // else if (estadoDist == 1) Serial.println("Obstáculo longe");
    // else if (estadoDist == 2) Serial.println("Obstáculo meia distância");
    // else Serial.println("Obstáculo perto");
    ultimoEstadoDist = estadoDist;
  }

  int ldrValue = analogRead(LDR_PIN);
  estadoLuz = (ldrValue < 60) ? 1 : 0;
  if (estadoLuz != ultimoEstadoLuz) {
    // Serial.print("Luminosidade mudou para: ");
    // if (estadoLuz == 0) Serial.println("Sem obstáculo acima");
    // else Serial.println("Obstáculo acima");
    ultimoEstadoLuz = estadoLuz;
  }

  float h = dht.readHumidity();     
  if (!isnan(h)) {
    estadoUmid = (h > 70.0) ? 1 : 0;
    if (estadoUmid != ultimoEstadoUmid) {
      // Serial.print("Umidade mudou para: ");
      // if (estadoUmid == 0) Serial.println("Aceitável");
      // else Serial.println("Não aceitável");
      ultimoEstadoUmid = estadoUmid;
    }
  }
  // Serial.println(estadoDist & 0x01);
  // Serial.println((estadoDist >> 1) & 0x01);
  // Serial.println(estadoLuz);
  // Serial.println(estadoUmid);
  // Serial.println(" ");
  
  digitalWrite(DIST_PIN0, estadoDist & 0x01);
  digitalWrite(DIST_PIN1, (estadoDist >> 1) & 0x01);
  digitalWrite(LUZ_PIN, estadoLuz);
  digitalWrite(UMID_PIN, estadoUmid);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(DATAPATH1_PIN, INPUT);
  pinMode(DATAPATH2_PIN, INPUT);

  pinMode(DIST_PIN0, OUTPUT);
  pinMode(DIST_PIN1, OUTPUT);
  pinMode(LUZ_PIN, OUTPUT);
  pinMode(UMID_PIN, OUTPUT);

  dht.begin();

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

  if (!digitalRead(DATAPATH1_PIN) && digitalRead(DATAPATH2_PIN)) {
    cleaning();
  }
  if (digitalRead(DATAPATH1_PIN) && !digitalRead(DATAPATH2_PIN)) {
    docking();
  }
  if (digitalRead(DATAPATH1_PIN) && digitalRead(DATAPATH2_PIN)) {
    charging();
  }
  if (!digitalRead(DATAPATH1_PIN) && !digitalRead(DATAPATH2_PIN)) {
    idle();
  }

  lerSensores();

  if (millis() - timerPrint >= INTERVALO_PRINT) {
    Serial.println("---------- STATUS ----------");
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

    Serial.println("----------------------------");
    timerPrint = millis();
  }
}
