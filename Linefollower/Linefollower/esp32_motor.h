#ifndef ESP32_MOTOR_H_
#define ESP32_MOTOR_H_

#pragma message("esp8266_motor.h included")

#define MOTOR_PIN_M1A 14
#define MOTOR_PIN_M1B 12

#define MOTOR_PIN_M2A 26
#define MOTOR_PIN_M2B 25

#define MOTOR_ENABLE 27

void Motor_Init(void);
void Motor_DeInit(void);
void Motor_SetSpeed(int16_t M1Speed, int16_t M2Speed);
void Motor_SetSpeedM1(int16_t M1Speed);
void Motor_SetSpeedM2(int16_t M2Speed);

void Motor_Init() {
  pinMode(MOTOR_ENABLE, OUTPUT);
  ledcAttach(MOTOR_PIN_M1A, 5000, 8);
  ledcAttach(MOTOR_PIN_M1B, 5000, 8);
  ledcAttach(MOTOR_PIN_M2A, 5000, 8);
  ledcAttach(MOTOR_PIN_M2B, 5000, 8);
}

void Motor_DeInit() {
  digitalWrite(MOTOR_ENABLE, LOW);
  ledcDetach(MOTOR_PIN_M1A);
  ledcDetach(MOTOR_PIN_M1B);
  ledcDetach(MOTOR_PIN_M2A);
  ledcDetach(MOTOR_PIN_M2B);
}

void Motor_SetSpeedM1(int16_t M1Speed) {

  uint8_t reverse = 0;

  if (M1Speed < 0) {
    M1Speed = -M1Speed;
    reverse = 1;
  }

  if (M1Speed > 0xFF) {
    M1Speed = 0xFF;
  }

  if (reverse) {
    ledcWrite(MOTOR_PIN_M1A, 0);
    ledcWrite(MOTOR_PIN_M1B, M1Speed);
  } else {
    ledcWrite(MOTOR_PIN_M1A, M1Speed);
    ledcWrite(MOTOR_PIN_M1B, 0);
  }
}

void Motor_SetSpeedM2(int16_t M2Speed) {

  uint8_t reverse = 0;

  if (M2Speed < 0) {
    M2Speed = -M2Speed;
    reverse = 1;
  }

  if (M2Speed > 0xFF) {
    M2Speed = 0xFF;
  }

  if (reverse) {
    ledcWrite(MOTOR_PIN_M2A, 0);
    ledcWrite(MOTOR_PIN_M2B, M2Speed);
  } else {
    ledcWrite(MOTOR_PIN_M2A, M2Speed);
    ledcWrite(MOTOR_PIN_M2B, 0);
  }
}

void Motor_SetSpeed(int16_t M1Speed, int16_t M2Speed) {

  if (M1Speed == 0 && M2Speed == 0) {
    digitalWrite(MOTOR_ENABLE, LOW);
  } else {
    digitalWrite(MOTOR_ENABLE, HIGH);
  }

  Motor_SetSpeedM1(M1Speed);
  Motor_SetSpeedM2(M2Speed);
}

#endif 
