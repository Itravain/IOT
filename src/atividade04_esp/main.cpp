#include <Arduino.h>
#include <Wire.h>

enum Estado{menu, query};

Estado estado = menu;
char command;
void setup() {

  Serial.begin(9600);
  Wire.begin();  //Talvez seja necessário definir os pinos
}

void loop() {
  
  switch (estado)
  {
    case menu:
    Serial.print("(t) Temperatura\n(h) Umidade\n(d) Distância\n");  
    if(Serial.available() > 0){
      command = Serial.read();
      estado = query;
    }
    break;
    
    case query:
      Wire.beginTransmission(8);
      Wire.write((byte)command);
      Wire.endTransmission();
      Wire.requestFrom(8, sizeof(float));

      float valor;
      byte *p = (byte*)&valor;
      for (int i = 0; i < sizeof(float); i++) {
        if (Wire.available()) p[i] = Wire.read();
      }

      Serial.print("Valor recebido: ");
      Serial.print(valor);
      
      estado = menu;
    break;
    
    default:
    break;
  }
}
