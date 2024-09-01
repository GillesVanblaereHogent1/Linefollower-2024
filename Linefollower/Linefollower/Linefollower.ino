#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include "driver/periph_ctrl.h"
#include <driver/timer.h>
#include <hal/timer_types.h>

#include "esp32_qtr.h"
#include "esp32_helper.h"
#include "esp32_motor.h"
#include "esp32_setparams.h"

void LFR_Initialize() {
  uint8_t QTR_Pins[] = { QTR_8_PIN, QTR_7_PIN, QTR_6_PIN, QTR_5_PIN, QTR_4_PIN, QTR_3_PIN, QTR_2_PIN, QTR_1_PIN };
  QTR_Init(QTR_Pins, QTR_EMITTER_PIN); /*start QTR-8RC Sensor */
  Motor_Init();                        /* Start Motoren */
}

void setup() {

  LFR_Initialize(); 

  NVM.begin("LINEFOLLOW", false);

  Lfr_Kp = NVM.getDouble("Lfr_Kp", LFR_KP);
  Lfr_Ki = NVM.getDouble("Lfr_Ki", LFR_KI);
  Lfr_Kd = NVM.getDouble("Lfr_Kd", LFR_KD);

  TargetLFRSpeed = NVM.getUShort("TargetLFRSpeed", LFR_MAX_MOTOR_SPEED);

  NVM.end();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(BAUD_RATE);
  Serial.println("---- Program Started ----");
  Serial.printf("setup() running on core %d \n", xPortGetCoreID());
  Serial.printf("Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
  Serial.printf("TargetLFRSpeed = %u \n", TargetLFRSpeed);

  Serial.printf("Connecting to WiFI : %s \n", ssid);
  WiFi.begin(ssid, password);
  uint8_t try_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    try_count++;
    if (try_count > 120) {
      ESP.restart();
    }
  }
  Serial.printf("\nWiFi connected..! Got IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_homepage);
  server.on("/get", handle_getdata);
  server.on("/resetbutton", handle_resetbutton);
  server.on("/clearnvm", handle_clearnvm);

  server.on("/runwhiteline", handle_runwhiteline);
  server.on("/runblackline", handle_runblackline);

  server.on("/motoroff", handle_motoroff);
  server.on("/motoron", handle_motoron);

  server.onNotFound(handle_notfound);

  server.begin();
  Serial.println("HTTP Server Started");

  xTaskCreatePinnedToCore(
    WiFiTaskCode, 
    "WiFiTask",   
    10000,       
    NULL,        
    1,            
    &WiFiTask,    
    0);          
}

void WiFiTaskCode(void* pvParameters) {

  Serial.print("WiFiTaskCode running on core ");
  Serial.println(xPortGetCoreID());

  while (1) {
    server.handleClient();
    vTaskDelay(50);
  }
}

void loop() {

  vTaskDelay(1000);
  Serial.print("loop() running on core ");
  Serial.println(xPortGetCoreID());
  Serial.println("Select LINE COLOUR in web interface ..... ");

  while (lineColour == LINE_UNKNOWN) {
    delay(250);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  if (lineColour == LINE_BLACK) {
    Serial.println("\nBLACK LINE COLOUR SELECTED READY TO FOLLOW, GET SET GO >>>>>>>>>>>");
  } else {
    Serial.println("\nWHITE LINE COLOUR SELECTED READY TO FOLLOW, GET SET GO >>>>>>>>>>>");
  }

  unsigned long LastTime = millis();

  for (int i = 10; i > 0; i--) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(i * 100);
  }

  while (1) {

    if (motorsRunning) {

      if ((millis() - LastTime) > 1000) {
        LastTime = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      }

      if (lineColour == LINE_WHITE) {
        Position = QTR_ReadWhiteLine(LFR_SensorValue, QTR_EMITTERS_ON); /*lijn positie */
      } else if (lineColour == LINE_BLACK) {
        Position = QTR_ReadBlackLine(LFR_SensorValue, QTR_EMITTERS_ON); 
      } else {
        Serial.println("LINE COLOUR ERROR , Rebooting Now");
        ESP.restart();
      }

      LFR_PrintSensorValues();

      if (Position.LFR_Position == ON_LINE) {

        CurrentLFRState = LFR_RUNNING;

        LFR_Proportional = Position.Sensor_Position - QTR_LINE_MID_VALUE; /*P*/
        LFR_Derivative = LFR_Proportional - LFR_LastProportional;         /*D */
        LFR_Integral += LFR_Proportional;                                 /*I*/
        LFR_LastProportional = LFR_Proportional;                          

        LFR_ControlOutput = LFR_Proportional * Lfr_Kp + LFR_Integral * Lfr_Ki + LFR_Derivative * Lfr_Kd; 

        if (LFR_ControlOutput > TargetLFRSpeed) {
          LFR_ControlOutput = TargetLFRSpeed; /*motorsnelheid binnen limiet */
        }
        if (LFR_ControlOutput < -TargetLFRSpeed) {
          LFR_ControlOutput = -TargetLFRSpeed; 
        }

        if (LFR_ControlOutput < 0) {
          Motor_SetSpeed(TargetLFRSpeed + LFR_ControlOutput, TargetLFRSpeed); 
        } else {
          Motor_SetSpeed(TargetLFRSpeed, TargetLFRSpeed - LFR_ControlOutput); 
        }

      } else if (Position.LFR_Position == ON_WHITE) {
        Serial.println("Line Follower ON ALL WHITE");
        CurrentLFRState = LFR_OUTSIDE;
        //motorsRunning = false;
        //SetParams();
        Motor_SetSpeed(0, 0);
        delay(5000);
      } else if (Position.LFR_Position == ON_BLACK) {
        Serial.println("Line Follower ON ALL BLACK");
        CurrentLFRState = LFR_BLACK;
        //motorsRunning = false;
        //SetParams();
        Motor_SetSpeed(0, 0);
        delay(5000);
      }

    } else {
      Motor_SetSpeed(0, 0);
      printIfNew("Line Follower STOP");
      CurrentLFRState = LFR_STOPPED;
      SetParams();
      //LFR_PrintSensorValues();
      vTaskDelay(500);
    }
  }
}
