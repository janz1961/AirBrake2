#include <EEPROM.h>

#include <RCReceive.h>
#include <Servo.h>        //Servo library

const byte pinReceiver = 2; 
const byte pinservo1 = 3; 
const byte pinservo2 = 4; 
const byte pinSwitch = 5; 
const byte pinLED = 13;
    
RCReceive rcReceiver;
Servo servo1;
Servo servo2;

byte min1 = 70;
byte max1 = 110;
byte min2 = 70;
byte max2 = 110;

void setup() {
  // Serial.begin(115200);
  // Serial.println("Start logging. Version " __DATE__ " " __TIME__);
  pinMode(pinSwitch, INPUT_PULLUP);

  // Serial.println("LED on (setup)");
  digitalWrite(pinLED, 255);

  min1 = EEPROM.read(0);
  max1 = EEPROM.read(1);
  min2 = EEPROM.read(2);
  max2 = EEPROM.read(3);

  // Serial.print("Load [");
  // Serial.print(min1);
  // Serial.print(", ");
  // Serial.print(max1);
  // Serial.print(", ");
  // Serial.print(min2);
  // Serial.print(", ");
  // Serial.print(max2);
  // Serial.println("]");

  // put your setup code here, to run once:
  rcReceiver.attach(pinReceiver);
  servo1.attach(pinservo1);
  servo2.attach(pinservo2);

  servo1.write(min1);
  delay(100);
  servo1.write(max2);
  delay(200);

  servo1.write(max1);
  delay(100);
  servo2.write(min2);
  delay(500);

  servo1.write(90);
  servo2.write(90);
  
  // Serial.println("LED off (setup)");
  digitalWrite(pinLED, 0);
}

enum STATE { operating, startservo1, updatingservo1, startservo2, updatingservo2, store } state;

long millisNow;

long millisPressed = 0;
bool buttonActive = false;
int valLast = 0;

void loop() {
  millisNow = millis();

  if (digitalRead(pinSwitch) == LOW) {
    if (millisPressed == 0) {
      millisPressed = millisNow;
    }
    else {
      if (!buttonActive && (millisNow - millisPressed) > 500) {
        buttonActive = true;
        // Serial.println("LED on");
        digitalWrite(pinLED, 255);
      }
    }
  }
  else {
    if (millisPressed != 0) {
      // Serial.println("LED off");
      digitalWrite(pinLED, 0);
  
      millisPressed = 0;
      if (buttonActive) {
        switch (state)
        {
          case operating:
            // Serial.println("Center before update servo 1");
            state = startservo1;
            break;

          case updatingservo1:
            // Serial.println("Center before update servo 2");
            state = startservo2;
            break;
            
          case updatingservo2:
            // Serial.println("Start saving parameters");
            state = store;
            break;

           default:
            // Serial.println("Shouldn't happen");
            break;
        }
      }
      buttonActive = false;
    }
  }
  rcReceiver.poll();
  int val = rcReceiver.getValue();
  
  switch (state)
  {
  case startservo1:
    // Make sure to start from center
    min1 = 70;
    max1 = 110;
    
    if (val > 120 && val < 136) {
      // Serial.println("Start update servo 1");
      state = updatingservo1;

      servo1.write(min1);
      delay(100);
      servo1.write(max1);
      delay(250);
    }
    break;
    
  case updatingservo1:
    val = map(val, 0, 255, 0, 180);
    servo1.write(val);
    if (val > max1) max1 = val;
    if (val < min1) min1 = val;
    delay(50);
    break;
    
  case startservo2:
    // Make sure to start from center
    min2 = 70;
    max2 = 110;

    if (val > 120 && val < 136) {
      // Serial.println("Start update servo 2");
      state = updatingservo2;

      servo2.write(min2);
      delay(100);
      servo2.write(max2);
      delay(250);
    }
    break;
    
  case updatingservo2:
    val = map(val, 0, 255, 0, 180);
    servo2.write(val);
    if (val > max2) max2 = val;
    if (val < min2) min2 = val;
    delay(50);
    break;

  case store:
    // Serial.print("Store [");
    // Serial.print(min1);
    // Serial.print(", ");
    // Serial.print(max1);
    // Serial.print(", ");
    // Serial.print(min2);
    // Serial.print(", ");
    // Serial.print(max2);
    // Serial.println("]");

    EEPROM.update(0, min1);
    EEPROM.update(1, max1);
    EEPROM.update(2, min2);
    EEPROM.update(3, max2);
    state = operating;

    servo1.write(min1);
    servo2.write(min2);
    delay(250);
    servo1.write(max1);
    servo2.write(max2);
    delay(250);
    break;

  case operating:
  default:
    if (abs(valLast - val) < 3) break;
    
    valLast = val;
  
    servo1.write(map(val, 0, 255, min1, max1));
    servo2.write(map(val, 255, 0, min2, max2));
    delay(10);
    break;
  }
}
