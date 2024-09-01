// Motor A
const int PWMA = 3;
const int AIN1 = 4;
const int AIN2 = 5;

// Motor B
const int PWMB = 6;
const int BIN1 = 7;
const int BIN2 = 8;
const int STBY = 9;

void setup() {

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);
  
  Serial.begin(9600);
 
  Serial.println("Format: M<MotorNumber> <Direction> <Speed>");
  Serial.println("MotorNumber: 1 or 2");
  Serial.println("Direction: F (Forward) or R (Reverse)");
  Serial.println("Speed: 0 to 255");
  Serial.println("Example: M1 F 200 (Motor 1 Forward at speed 200)");
}

void loop() {
  if (Serial.available() > 0) {
    
    String input = Serial.readStringUntil('\n');
    input.trim();  

    char motor = input.charAt(1);  // M1 or M2
    char direction = input.charAt(3);  // F or R
    int speed = input.substring(5).toInt();  // Speed 

    //(M1)
    if (motor == '1') {
      if (direction == 'F') {
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
      } else if (direction == 'R') {
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, HIGH);
      }
      analogWrite(PWMA, speed);
    }
    
    //(M2)
    else if (motor == '2') {
      if (direction == 'F') {
        digitalWrite(BIN1, HIGH);
        digitalWrite(BIN2, LOW);
      } else if (direction == 'R') {
        digitalWrite(BIN1, LOW);
        digitalWrite(BIN2, HIGH);
      }
      analogWrite(PWMB, speed);
    }
    Serial.println("Command received: " + input);
  }
}
