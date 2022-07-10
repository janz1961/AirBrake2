#include <EEPROM.h>

#include <RCReceive.h>
#include <Servo.h>        //Servo library

void LED(bool on = true);

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
  Serial.begin(115200);
  Serial.println("Start logging. Version " __DATE__ " " __TIME__);
  pinMode(pinSwitch, INPUT_PULLUP);

  Serial.println("LED on (setup)");
  digitalWrite(pinLED, 255);

  min1 = EEPROM.read(0);
  max1 = EEPROM.read(1);
  min2 = EEPROM.read(2);
  max2 = EEPROM.read(3);

  Serial.print("Load [");
  Serial.print(min1);
  Serial.print(", ");
  Serial.print(max1);
  Serial.print(", ");
  Serial.print(min2);
  Serial.print(", ");
  Serial.print(max2);
  Serial.println("]");

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
  
  Serial.println("LED off (setup)");
  digitalWrite(pinLED, 0);
}

enum STATE { startoperating, operating, startservo1, updatingservo1, startservo2, updatingservo2, store } state = startoperating;

long millisNow;

byte LEDStatus = 0;
long millisPressed = 0;
long millisBlinkStart = 0;
int blinkInterval = 1000;
bool buttonActive = false;
uint8_t valLast = 0;

void loop() {
  millisNow = millis();

  if (millisBlinkStart > 0 && (millisNow - millisBlinkStart) > blinkInterval) {
    // Is blinking. Start new blink with lED on
    millisBlinkStart = millisNow;
    LED();
  }

  if (digitalRead(pinSwitch) == LOW) {
    // Button is pressed. Do nothing except check duration of press and controlling LED
    if (millisPressed == 0) {
      // Button pressed: turn LED off, but leave blink timing intact
      millisPressed = millisNow;
      LED(false);
      return;
    }

    if (buttonActive || (millisNow - millisPressed) < 500) return;
    
    // Activate button function, show by turning on LED
    buttonActive = true;
    millisBlinkStart = 0;
    LED();
    return;
  }
  
  if (millisPressed != 0) {
    // Button has been released (after having been pressed)
    LED(false);

    millisPressed = 0;
    if (buttonActive) {
      // Button has bee pressed longer thna threshold
      switch (state)
      {
        case operating:
          Serial.println("Center before update servo 1");
          state = startservo1;
          break;

        case updatingservo1:
          Serial.println("Center before update servo 2");
          state = startservo2;
          break;
          
        case updatingservo2:
          Serial.println("Start saving parameters");
          state = store;
          break;

        default:
          Serial.println("Shouldn't happen");
          break;
      }
    }
    buttonActive = false;
  }

  rcReceiver.poll();
  uint8_t val = rcReceiver.getValue();

  switch (state)
  {
  case startservo1:
    // Make sure to start from center
    min1 = 70;
    max1 = 110;
    millisBlinkStart = 0;
    blinkInterval = 1000;
    
    if (val > 120 && val < 136) {
      Serial.println("Start update servo 1");
      state = updatingservo1;

      servo1.write(min1);
      Serial.println(min1);
      delay(100);
      Serial.println(max1);
      servo1.write(max1);
      delay(250);

      // Start blinking
      millisBlinkStart = millisNow;
      LED();
    }
    break;
    
  case updatingservo1:
    if ((millisNow - millisBlinkStart) > 100) LED(false);

    if (abs(valLast - val) < 3) break;;
    
    valLast = val;
    val = map(val, 0, 255, 0, 180);
    Serial.println(val);
    servo1.write(val);
    if (val > max1) max1 = val;
    if (val < min1) min1 = val;
    break;
    
  case startservo2:
    // Make sure to start from center
    min2 = 70;
    max2 = 110;
    millisBlinkStart = 0;
    blinkInterval = 1000;

    if (val > 120 && val < 136) {
      Serial.println("Start update servo 2");
      state = updatingservo2;

      Serial.println(min2);
      servo2.write(min2);
      delay(100);
      Serial.println(max2);
      servo2.write(max2);
      delay(250);

      // Start blinking
      millisBlinkStart = millisNow;
      LED();
    }
    break;
    
  case updatingservo2:
    if ((millisNow - millisBlinkStart) > 100) LED(false);
    if ((millisNow - millisBlinkStart) > 200) LED();
    if ((millisNow - millisBlinkStart) > 300) LED(false);

    if (abs(valLast - val) < 3) break;

    valLast = val;
    val = map(val, 0, 255, 0, 180);
    Serial.println(val);
    servo2.write(val);
    if (val > max2) max2 = val;
    if (val < min2) min2 = val;
    break;

  case store:
    // Stop blinking, turn LED on
    millisBlinkStart = 0;
    LED();

    Serial.print("Store [");
    Serial.print(min1);
    Serial.print(", ");
    Serial.print(max1);
    Serial.print(", ");
    Serial.print(min2);
    Serial.print(", ");
    Serial.print(max2);
    Serial.println("]");

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
    
    // Turn LED off
    LED(false);

    state = startoperating;
    break;

  case startoperating:
    millisBlinkStart = millisNow;
    blinkInterval = 5000;
    LED(false);

    state = operating;    
    break;

  case operating:
  default:
    if ((millisNow - millisBlinkStart) > 50) LED(false);

    if (abs(valLast - val) < 3) return;
    
    valLast = val;
    
    Serial.print("Input is: ");
    Serial.println(val);
  
    int x1 = map(val, 0, 255, min1, max1);
    int x2 = map(val, 255, 0, min2, max2);

    Serial.print(x1);
    Serial.print(":");
    Serial.print(x2);
    Serial.println("");
    
    servo1.write(x1);
    servo2.write(x2);
    break;
  }
}

void LED(bool on = true) {
  if (on && LEDStatus == 0) {
    Serial.println("LED on");
    LEDStatus = 255;
  }
  else 
  if (!on && LEDStatus == 255) {
    Serial.println("LED off"); 
    LEDStatus = 0;
  }

  digitalWrite(pinLED, LEDStatus);
}
