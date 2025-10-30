#include <Arduino.h>
#include <Wire.h>

const int trigPin = 53;
const int echoPin = 52;

#define SOUND_SPEED 0.034

long duration;
float distanceCm;

#include "DHT.h"
#define DHTPIN 2 // what digital pin we're connected to
#define DHTTYPE DHT11 // DHT11
DHT dht(DHTPIN, DHTTYPE);

float temperature;
float humidity;

char command;

void getDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  
  distanceCm = duration * SOUND_SPEED/2;
  
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);

  
  delay(1000);
}

void getDHT()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void receiveEvent(int howMany) {
  if(Wire.available()){
    command = (char)Wire.read();
    Serial.print("Recebido ");
    Serial.print(command);
  }

}

void requestEvent(){
  switch (command)
  {
  case 'd':
    Wire.write((byte*)&distanceCm, sizeof(float));
    break;
  case 't':
    Wire.write((byte*)&temperature, sizeof(float));
    break;
  case 'h':
    Wire.write((byte*)&humidity, sizeof(float));
    break;
  default:
    float erro = -1.0;
    Wire.write((byte*)&erro, sizeof(float));
    break;
  }  
}

void setup() {
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  Serial.begin(115200); 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  dht.begin();
}

void loop() {
  delay(100);
  getDistancia();
  getDHT();
}


