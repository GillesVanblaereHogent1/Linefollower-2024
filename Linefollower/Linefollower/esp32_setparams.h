
void readDoublesFromSerial(double* var1, double* var2, double* var3) {

  Serial.println("Please enter three double values separated by spaces.");

  char input[64];
  bool validInput = false;

  while (!validInput) {
    Serial.print("Enter values: ");
    int index = 0;
    while (1) {
      if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
          break;
        }
        if (index < sizeof(input) - 1) {
          input[index++] = c;
        } else {
          break;
        }
      }
    }

    input[index] = '\0';

    int numParsed = sscanf(input, "%lf %lf %lf", var1, var2, var3);
    if (numParsed == 3) {
      validInput = true;
    } else {
      Serial.println("Invalid input. Please enter three double values separated by spaces.");
      Serial.flush();
    }
  }
}

void SetParams() {

  Motor_SetSpeed(0, 0);

  LFR_Integral = 0;
  LFR_LastProportional = 0;
  LFR_Derivative = 0;

  Serial.println("w - FRONT; s - BACK; a - LEFT; d - RIGHT; f - FOLLOW LINE; x - SENSOR_VALUES; v - SET LFR_PID; c - CLEAR NVM; r - RESET DEVICE");
  LFR_PrintSensorValues();
  bool stopLoop = false;

  while (!stopLoop & !motorsRunning) {
    if (Serial.available() > 0) {
      char command = Serial.read();  

      switch (command) {
        case 'w':  // Move Forward
          Motor_SetSpeed(20, 20);
          break;
        case 's':  // Move Backward
          Motor_SetSpeed(-20, -20);
          break;
        case 'a':  // Turn Left
          Motor_SetSpeed(-20, 20);
          break;
        case 'd':  // Turn Right
          Motor_SetSpeed(20, -20);
          break;
        case 'v':

          NVM.begin("LINEFOLLOW", false);
          Serial.printf("CURRENT VALUES = Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
          readDoublesFromSerial(&Lfr_Kp, &Lfr_Kd, &Lfr_Ki);
          NVM.putDouble("Lfr_Kp", Lfr_Kp);
          NVM.putDouble("Lfr_Ki", Lfr_Ki);
          NVM.putDouble("Lfr_Kd", Lfr_Kd);
          Serial.printf("SET VALUES = Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
          NVM.end();

          break;
        case 'c':  // Clear NVM

          NVM.begin("LINEFOLLOW", false);
          NVM.clear();
          NVM.end();

          break;
        case 'r':
          ESP.restart();
          break;
        case '\n':
        case '\r':
          continue;
          break;
        case 'f':  // Start Line Following
          motorsRunning = true;
          stopLoop = true;
          break;
        default:
          Motor_SetSpeed(0, 0);
      }

      delay(500);
      Motor_SetSpeed(0, 0);

      if (lineColour == LINE_WHITE) {
        Position = QTR_ReadWhiteLine(LFR_SensorValue, QTR_EMITTERS_ON); 
      } else if (lineColour == LINE_BLACK) {
        Position = QTR_ReadBlackLine(LFR_SensorValue, QTR_EMITTERS_ON); 
      } else {
        Serial.println("LINE COLOUR ERROR , Rebooting Now");
        ESP.restart();
      }
      LFR_PrintSensorValues();
      Serial.println("w - FRONT; s - BACK; a - LEFT; d - RIGHT; f - FOLLOW LINE; x - SENSOR_VALUES; v - SET LFR_PID; c - CLEAR NVM; r - RESET DEVICE");

    } else {
      Motor_SetSpeed(0, 0);  
    }
  }
}
