#define IDLE 0
#define CLEAN 1
#define DOCK 2
#define CHRG 3

#define LED1_PIN 2
#define LED2_PIN 3

int estadoAtual = IDLE;
unsigned long timerEstado;
unsigned long timerLED1;
unsigned long timerLED2;
boolean estadoLED1 = false;
boolean estadoLED2 = false;

#define TEMPO_TRANSICAO 5000
#define INTERVALO_LED1 100
#define INTERVALO_LED2 50

void setup() {
  Serial.begin(9600);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  
  timerEstado = millis();
  timerLED1 = millis();
  timerLED2 = millis();
  
  Serial.println("Roomba Inicializado!");
  Serial.println("Comandos: 'a' = Clean, 'b' = Dock");
  Serial.println("Estado atual: IDLE");
}

void loop() {
  char comando = lerComando();
  
  switch (estadoAtual) {
    case IDLE:
      idle();
      if (comando == 'a') {
        mudarEstado(CLEAN);
      }
      break;
      
    case CLEAN:
      cleaning();
      if (comando == 'b') {
        mudarEstado(DOCK);
      } else if (millis() - timerEstado >= TEMPO_TRANSICAO) {
        mudarEstado(DOCK);
      }
      break;
      
    case DOCK:
      docking();
      if (millis() - timerEstado >= TEMPO_TRANSICAO) {
        mudarEstado(CHRG);
      }
      break;
      
    case CHRG:
      charging();
      if (comando == 'a') {
        mudarEstado(CLEAN);
      } else if (millis() - timerEstado >= TEMPO_TRANSICAO) {
        mudarEstado(IDLE);
      }
      break;
      
    default:
      mudarEstado(IDLE);
      break;
  }
}

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
}

void cleaning() {
  digitalWrite(LED2_PIN, LOW);
  
  if (millis() - timerLED1 >= INTERVALO_LED1) {
    estadoLED1 = !estadoLED1;
    digitalWrite(LED1_PIN, estadoLED1);
    timerLED1 = millis();
  }
}

void docking() {
  digitalWrite(LED1_PIN, LOW);
  
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