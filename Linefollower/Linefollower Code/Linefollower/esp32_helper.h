#define LED_BUILTIN 2

#define LFR_KP 0.3 // Controls the robot's immediate response to the error. Higher values lead to faster responses but can cause oscillations.
#define LFR_KI 0.000005 //  Eliminates steady-state error by considering the cumulative error over time. Higher values can correct small persistent errors but may cause instability if too high.
#define LFR_KD 4.00000  // Smooths the response by considering the rate of change of the error. Higher values reduce oscillations but can slow down the response and make the system sensitive to noise. 

#define LFR_MAX_MOTOR_SPEED 120  // Sets the Maximum PWM Duty Cycle for Line Follower Robot 0=0% 255=100%

#define BAUD_RATE 115200 

typedef enum {
  LFR_UNKNOWN,
  LFR_RUNNING,
  LFR_OUTSIDE,
  LFR_BLACK,
  LFR_STOPPED
} lfrstates_t;

lfrstates_t CurrentLFRState = LFR_UNKNOWN;

QTR_Position Position;                      /* Variable to Save the Sensor and Robot Positie */
uint16_t LFR_Position = 0;                  
int16_t LFR_Proportional = 0;               
int16_t LFR_LastProportional = 0;           
int16_t LFR_Derivative = 0;                 
int64_t LFR_Integral = 0;                   
int16_t LFR_ControlOutput = 0;              

uint16_t TargetLFRSpeed = LFR_MAX_MOTOR_SPEED;

double Lfr_Kp = LFR_KP;
double Lfr_Ki = LFR_KI;
double Lfr_Kd = LFR_KD;

const char *ssid = "@#$%^&";              
const char *password = "12345678713101";  

const char *PARAM_INPUT_1 = "Lfr_Kp";
const char *PARAM_INPUT_2 = "Lfr_Ki";
const char *PARAM_INPUT_3 = "Lfr_Kd";

volatile bool motorsRunning = true;

typedef enum {
  LINE_UNKNOWN,
  LINE_BLACK,
  LINE_WHITE
} linecolour_t;

volatile linecolour_t lineColour = LINE_UNKNOWN;

uint16_t LFR_SensorValue[QTR_SENSOR_COUNT]; 

Preferences NVM;
WebServer server(80);
TaskHandle_t WiFiTask;

void OnWhiteCode(void);
void OnLineCode(void);
void OnBlackLine(void);

void printIfNew(const char *text);
void printOncePerSecond(const char *text);
void printOncePerFiveSecond(const char *text);

String SendHTML() {

  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>LFR Control</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr += ".button {display: block;width: 200px;background-color: #3498db;border: none;color: white;padding: 10px 20px;text-decoration: none;font-size: 14px;margin: 0px auto 20px;cursor: pointer;border-radius: 4px;}\n";
  ptr += ".button-on {background-color: #3498db;}\n";
  ptr += ".button-on:active {background-color: #2980b9;}\n";
  ptr += ".button-off {background-color: #34495e;}\n";
  ptr += ".button-off:active {background-color: #2c3e50;}\n";
  ptr += "p {font-size: 25px;color: #000;font-weight: bold;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<h1>ESP32 LFR Server</h1>\n";
  ptr += "<a class=\"button button-on\" href=\"/\">HOME PAGE</a>\n";

  ptr += "<form action=\"/get\">Lfr_Kp: ";
  ptr += String(Lfr_Kp, 6);
  ptr += " <input type=\"text\" name=\"Lfr_Kp\"><input type=\"submit\" value=\"Submit\">\</form><br>";
  ptr += "<form action=\"/get\">Lfr_Ki: ";
  ptr += String(Lfr_Ki, 6);
  ptr += " <input type=\"text\" name=\"Lfr_Ki\"><input type=\"submit\" value=\"Submit\">\</form><br>";
  ptr += "<form action=\"/get\">Lfr_Kd: ";
  ptr += String(Lfr_Kd, 6);
  ptr += " <input type=\"text\" name=\"Lfr_Kd\"><input type=\"submit\" value=\"Submit\">\</form><br>";

  ptr += "<form action=\"/get\">TargetLFRSpeed: ";
  ptr += String(TargetLFRSpeed);
  ptr += " <input type=\"text\" name=\"TargetLFRSpeed\"><input type=\"submit\" value=\"Submit\">\</form><br>";
  
  ptr += "<a class=\"button button-white\" href=\"/runwhiteline\">START WHITE LINE</a>\n";
  ptr += "<a class=\"button button-black\" href=\"/runblackline\">START BLACK LINE</a>\n";

  if (motorsRunning) {
    ptr += "<p>Motor Running Status: ON</p><a class=\"button button-on\" href=\"/motoroff\">TURN OFF</a>\n";
  } else {
    ptr += "<p>Motor Running Status: OFF</p><a class=\"button button-off\" href=\"/motoron\">TURN ON</a>\n";
  }

  ptr += "<a class=\"button button-on\" href=\"/clearnvm\">CLEAR NVM</a>\n";
  ptr += "<a class=\"button button-on\" href=\"/resetbutton\">RESET ESP32</a>\n";

  if (CurrentLFRState == LFR_RUNNING) {
    ptr += "<p>LFR Status: LFR_RUNNING</p>\n";
  } else if (CurrentLFRState == LFR_OUTSIDE) {
    ptr += "<p>LFR Status: LFR_OUTSIDE</p>\n";
  } else if (CurrentLFRState == LFR_STOPPED) {
    ptr += "<p>LFR Status: LFR_STOPPED</p>\n";
  }  else if (CurrentLFRState == LFR_BLACK) {
    ptr += "<p>LFR Status: LFR_BLACK</p>\n";
  }else {
    ptr += "<p>LFR Status: LFR_UNKNOWN</p>\n";
  }

  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

void handle_homepage() {
  Serial.println("HTML Updated");
  server.send(200, "text/html", SendHTML());
}

void handle_resetbutton() {
  Serial.println("ESP32 Reboot ....");
  server.send(200, "text/html", SendHTML());
  ESP.restart();
}

void handle_clearnvm() {
  Serial.println("ESP32 NVM CLEAR ....");
  server.send(200, "text/html", SendHTML());
  
  NVM.begin("LINEFOLLOW", false);
  NVM.clear();
  NVM.end();
  
}

void handle_motoroff() {
  motorsRunning = false;
  Serial.println("Motor Running Status: OFF");
  server.send(200, "text/html", SendHTML());
}

void handle_motoron() {
  motorsRunning = true;
  Serial.println("Motor Running Status: ON");
  server.send(200, "text/html", SendHTML());
}

void handle_runwhiteline() {
  lineColour = LINE_WHITE;
  Serial.println("LFR Line Colour: LINE_WHITE");
  server.send(200, "text/html", SendHTML());
}

void handle_runblackline() {
  lineColour = LINE_BLACK;
  Serial.println("LFR Line Colour: LINE_BLACK");
  server.send(200, "text/html", SendHTML());
}

void handle_getdata() {
  String inputMessage;
  String inputParam;

  if (server.hasArg("Lfr_Kp")) {
    inputMessage = server.arg("Lfr_Kp");
    Lfr_Kp = inputMessage.toDouble();
    inputParam = "Lfr_Kp";
    NVM.begin("LINEFOLLOW", false);
    NVM.putDouble("Lfr_Kp", Lfr_Kp);
    Serial.printf("SET VALUES = Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
    NVM.end();
  } else if (server.hasArg("Lfr_Ki")) {
    inputMessage = server.arg("Lfr_Ki");
    Lfr_Ki = inputMessage.toDouble();
    inputParam = "Lfr_Ki";
    NVM.begin("LINEFOLLOW", false);
    NVM.putDouble("Lfr_Ki", Lfr_Ki);
    Serial.printf("SET VALUES = Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
    NVM.end();
  } else if (server.hasArg("Lfr_Kd")) {
    inputMessage = server.arg("Lfr_Kd");
    Lfr_Kd = inputMessage.toDouble();
    inputParam = "Lfr_Kd";
    NVM.begin("LINEFOLLOW", false);
    NVM.putDouble("Lfr_Kd", Lfr_Kd);
    Serial.printf("SET VALUES = Lfr_Kp = %f , Lfr_Ki = %f , Lfr_Kd = %f \n", Lfr_Kp, Lfr_Ki, Lfr_Kd);
    NVM.end();
  } else if (server.hasArg("TargetLFRSpeed")) {
    inputMessage = server.arg("TargetLFRSpeed");
    TargetLFRSpeed = inputMessage.toDouble();
    inputParam = "TargetLFRSpeed";
    NVM.begin("LINEFOLLOW", false);
    NVM.putUShort("TargetLFRSpeed", TargetLFRSpeed);
    Serial.printf("SET VALUES = TargetLFRSpeed = %u \n", TargetLFRSpeed);
    NVM.end();
  } else {
    inputMessage = "No Message Sent";
    inputParam = "NONE";
  }
  Serial.println("HTTP Get Data " + inputParam + " " + inputMessage);
  server.send(200, "text/html", SendHTML());
}

void handle_notfound() {
  server.send(404, "text/plain", "Not found");
}

void LFR_PrintSensorValues() {
  for (uint8_t i = 0; i < QTR_SENSOR_COUNT; i++) {
    Serial.print(LFR_SensorValue[i]);
    if (i != QTR_SENSOR_COUNT - 1) {
      Serial.print(" , ");
    }
  }
  Serial.println();
}

void printOncePerSecond(const char *text) {
  static unsigned long lastPrintTime1 = 0;  
  unsigned long currentTime = micros();     
  if (currentTime - lastPrintTime1 >= 1000000) {
    Serial.println(text);          
    lastPrintTime1 = currentTime;  
  }
}

void printOncePerFiveSecond(const char *text) {
  static unsigned long lastPrintTime5 = 0;  
  unsigned long currentTime = micros();     
  if (currentTime - lastPrintTime5 >= 5000000) {
    Serial.println(text);          
    lastPrintTime5 = currentTime;  
  }
}

void printIfNew(const char *text) {
  static char lastPrintedText[100] = "";  
  if (strcmp(text, lastPrintedText) != 0) {
    // If the new text is different from the last printed text
    Serial.println(text);                                         
    strncpy(lastPrintedText, text, sizeof(lastPrintedText) - 1);  
    lastPrintedText[sizeof(lastPrintedText) - 1] = '\0';          
  }
}
