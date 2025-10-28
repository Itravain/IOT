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

void receiveEvent() {
  char c;
  while (1 < Wire.available()) {
    c = Wire.read();
    Serial.print(c);
  }
  if (c == 't'){
    Wire.write(temperature);
  }
  
}

void setup() {
  Wire.begin(8);
  Wire.onReceive(receiveEvent);
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


