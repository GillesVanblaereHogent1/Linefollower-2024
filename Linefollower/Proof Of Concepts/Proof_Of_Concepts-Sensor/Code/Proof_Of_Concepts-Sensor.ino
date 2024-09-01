#include <QTRSensors.h>

const uint8_t sensorPins[] = {2, 3, 4, 5, 6, 7, 8, 9};

QTRSensorsRC qtrrc(sensorPins, sizeof(sensorPins) / sizeof(sensorPins[0]));

uint16_t sensorValues[8];

void setup() {
  Serial.begin(9600);
}

void loop() {
  qtrrc.read(sensorValues);

  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(sensorValues[i]);
    Serial.print('\t'); 
  }
  Serial.println();  

  delay(250); 
}
