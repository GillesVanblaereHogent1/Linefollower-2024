#ifndef ESP32_QTR_H_
#define ESP32_QTR_H_

#pragma message("esp8266_qtr.h included")

#define QTR_8_PIN 23
#define QTR_7_PIN 22
#define QTR_6_PIN 21
#define QTR_5_PIN 19
#define QTR_4_PIN 18
#define QTR_3_PIN 5
#define QTR_2_PIN 17
#define QTR_1_PIN 16
#define QTR_EMITTER_PIN 4

#define QTR_SENSOR_COUNT 8            /*set de actieve sensors*/
#define QTR_TIMEOUT 800               
#define QTR_MAX_VALUE QTR_TIMEOUT     
#define QTR_CALIBRATE_TIMES 10        
#define QTR_LINE_DETECT_THRESHOLD 150 /*Minimum Threshold lijn detectie */
#define QTR_SENSOR_THRESHOLD 50       
#define QTR_LINE_MID_VALUE 3500       

#define QTR_EMITTERS_OFF 0        
#define QTR_EMITTERS_ON 1         
#define QTR_EMITTERS_ON_AND_OFF 2 

typedef enum {
  UNKNOWN,
  ON_LINE,
  ON_WHITE,
  ON_BLACK
} Detection_Status;

typedef struct QTR_Position {
  uint16_t Sensor_Position; 
  Detection_Status LFR_Position;
} QTR_Position;


void QTR_Init(uint8_t *SensorPin, uint8_t EmitterPin);
void QTR_DeInit();
void QTR_ReadSensor(uint16_t *SensorValues, uint8_t ReadMode);
void IRAM_ATTR QTR_ReadRaw(uint16_t *SensorValues);
QTR_Position QTR_ReadWhiteLine(uint16_t *SensorValues, uint8_t ReadMode);
QTR_Position QTR_ReadBlackLine(uint16_t *SensorValues, uint8_t ReadMode);
static inline void QTR_EmitterOn();
static inline void QTR_EmitterOff();


uint8_t QTR_SensorPin[QTR_SENSOR_COUNT];
uint8_t QTR_EmitterPin;


void QTR_Init(uint8_t *SensorPin, uint8_t EmitterPin) {
  QTR_EmitterPin = EmitterPin;

  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    QTR_SensorPin[i] = SensorPin[i];
    pinMode(SensorPin[i], OUTPUT);
  }
  pinMode(EmitterPin, OUTPUT);
}


void QTR_DeInit() {
  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    pinMode(QTR_SensorPin[i], INPUT);
  }
  pinMode(QTR_EmitterPin, INPUT);
}


 
void IRAM_ATTR QTR_ReadRaw(uint16_t *SensorValues) {
  uint8_t i;
  uint64_t TotalTime = 0;
  uint64_t StartTime = 0;

  //noInterrupts();

  for (i = 0; i < QTR_SENSOR_COUNT; i++) {
    SensorValues[i] = QTR_TIMEOUT;
  }

  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    pinMode(QTR_SensorPin[i], OUTPUT);
     gpio_set_level((gpio_num_t)QTR_SensorPin[i], HIGH);
  }

  delayMicroseconds(10);

  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    pinMode(QTR_SensorPin[i], INPUT);
    gpio_set_level((gpio_num_t)QTR_SensorPin[i], LOW);
  }

  StartTime = micros();

  while (TotalTime < QTR_TIMEOUT) {
    TotalTime = micros() - StartTime;

    for (i = 0; i < QTR_SENSOR_COUNT; i++) {
      if ((digitalRead(QTR_SensorPin[i]) == LOW) && (TotalTime < SensorValues[i])) {
        SensorValues[i] = TotalTime;
      }
    }
  }

  //interrupts();
}


void QTR_ReadSensor(uint16_t *SensorValues, uint8_t ReadMode) {
  if (ReadMode == QTR_EMITTERS_ON || ReadMode == QTR_EMITTERS_ON_AND_OFF) {
    QTR_EmitterOn();
  }

  QTR_ReadRaw(SensorValues);
  QTR_EmitterOff();

  if (ReadMode == QTR_EMITTERS_ON_AND_OFF) {
    uint16_t OffSensorValues[QTR_SENSOR_COUNT];
    QTR_ReadRaw(OffSensorValues);
    for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
      SensorValues[i] += QTR_MAX_VALUE - OffSensorValues[i];
    }
  }
}


QTR_Position QTR_ReadBlackLine(uint16_t *SensorValues, uint8_t ReadMode) {
  uint8_t LineDetected = 0;
  uint32_t WeightedTotal = 0;
  uint16_t WeightSum = 0;
  QTR_Position LFR_Position;
  static int WeightedAverage = QTR_LINE_MID_VALUE;

  QTR_ReadSensor(SensorValues, ReadMode);

  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    uint16_t value = SensorValues[i];

    if (value > QTR_LINE_DETECT_THRESHOLD) {
      LineDetected++;
    }

    if (value > QTR_SENSOR_THRESHOLD) {
      // if (value < QTR_LINE_DETECT_THRESHOLD)
      // {
      //   value = QTR_SENSOR_THRESHOLD;
      // }

      WeightedTotal += value * i;
      WeightSum += value;
    }
  }

  if (!LineDetected) {
    LFR_Position.Sensor_Position = QTR_LINE_MID_VALUE;
    LFR_Position.LFR_Position = ON_WHITE;
    return LFR_Position;
  } else if (LineDetected == ((QTR_SENSOR_COUNT))) {
    LFR_Position.LFR_Position = ON_BLACK;
  } else {
    LFR_Position.LFR_Position = ON_LINE;
  }

  WeightedTotal = WeightedTotal * 1000;
  WeightedAverage = WeightedTotal / WeightSum;
  LFR_Position.Sensor_Position = WeightedAverage;

  return LFR_Position;
}


QTR_Position QTR_ReadWhiteLine(uint16_t *SensorValues, uint8_t ReadMode) {
  uint8_t LineDetected = 0;
  uint32_t WeightedTotal = 0;
  uint16_t WeightSum = 0;
  QTR_Position LFR_Position;
  static int WeightedAverage = QTR_LINE_MID_VALUE;

  QTR_ReadSensor(SensorValues, ReadMode);

  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    uint16_t value = QTR_TIMEOUT - SensorValues[i];

    if (value > QTR_LINE_DETECT_THRESHOLD) {
      LineDetected++;
    }

    if (value > QTR_SENSOR_THRESHOLD) {
      // if (value < QTR_LINE_DETECT_THRESHOLD)
      // {
      //   value = QTR_SENSOR_THRESHOLD;
      // }

      WeightedTotal += value * i;
      WeightSum += value;
    }
  }

  if (!LineDetected) {
    LFR_Position.Sensor_Position = QTR_LINE_MID_VALUE;
    LFR_Position.LFR_Position = ON_BLACK;
    return LFR_Position;
  } else if (LineDetected == ((QTR_SENSOR_COUNT))) {
    LFR_Position.LFR_Position = ON_WHITE;
  } else {
    LFR_Position.LFR_Position = ON_LINE;
  }

  WeightedTotal = WeightedTotal * 1000;
  WeightedAverage = WeightedTotal / WeightSum;
  LFR_Position.Sensor_Position = WeightedAverage;

  return LFR_Position;
}


static inline void QTR_EmitterOn() {
  gpio_set_level((gpio_num_t)QTR_EmitterPin, HIGH);
}


static inline void QTR_EmitterOff() {
  gpio_set_level((gpio_num_t)QTR_EmitterPin, LOW);
}

#endif 
