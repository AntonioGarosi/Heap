/*
1 - Buzz()
2 - UrgentBuzz()
3 - FailBuzz()
4 - ErrorBuzz()
5 - RestartBuzz()
*/

#include "HX711.h"
#include "Wire.h"

//#define POTENTIOMETERS //for future implementations

//moto pins
#define dc1 5
#define dc2 6

#define led 8

//load bar pins
#define DOUT  2
#define CLK  3

#ifdef POTENTIOMETERS
  #define pot1 A0
  #define pot2 A1
  #define pot3 A2
#endif

//pre-weighted values of cards and associated blocks of the very first prototype
//calculate and modify according your project
#define cards 10
const int cardWeights[cards][2] = {
  {0, 0}, //placeholder
  {14, 23}, //Ball
  {12, 16}, //Block notes & Pen
  {20, 36}, //Delivery box
  {22, 3}, //Shoes
  {32, 34}, //TV
  {6, 14}, //Bottle
  {23, 26}, //Drawing
  {9, 10}, //Book
  {17, 12} //Umbrella
};

//FSM state variable
enum State {start, normal, error, loading, lost};
State state;

float actualLoad;
float wrongLoad;
float tolerance = 1.0; //1g of load tolerance
int loadAssessment = 50; //50 ms of assessment time
int cardDetected;

HX711 scale(DOUT, CLK);
float calibration_factor = 210000;
bool errorFlag;

void RapidBlink() {
  digitalWrite(led, LOW);
  for (short i = 0; i < 4; i++) {
    digitalWrite(led, !digitalRead(led));
    delay(50);
  }
}

//Led blinks n times
void Blink(int n) {
  for (int k = 0; k < n; k++) {
    digitalWrite(led, HIGH);
    delay(400);
    digitalWrite(led, LOW);
    delay(400);
  }
}

//Motor shakes t milliseconds
void Shake(int t) {
  analogWrite(dc1, 255);
  delay(t);
  analogWrite(dc1, 0);
}

//Set alarm in speaker board
void SetAlarm(int n) {
 Wire.beginTransmission(1);
 Wire.write(n);
 Wire.endTransmission();
 RapidBlink();
}

//Request alarm from speaker board
void Alarm() {
  Wire.requestFrom(1,2); //2 byte from device 1
  while (Wire.available()) {
    int c = Wire.read();
    RapidBlink(); //positive feedback
    }
}

//Alarm remote control
void FireAlarm(int a) {
  SetAlarm(a);
  Alarm();
}

//Compares actual load against given value
bool CheckLoad(int value) {
  delay(loadAssessment); //load assessment delay
  float load = scale.get_units(10);
  if ((load > value - tolerance) && (load < value + tolerance)) {
    return true;
  }
  return false;
}

//Checks if load has varies
bool CheckLoadVariation(){
  float tmp = scale.get_units(10);
  if ((tmp > actualLoad  + tolerance) || (tmp < actualLoad - tolerance))
    return true;
  return false;
}

//Checks if a card has been loaded. Returns 0 if no card is matched
int CheckForCard() {
  for (short i = 0; i < cards; i++) {
   if (CheckLoad(cardWeights[i])) {
    return i; 
   }
  }
  return 0;
}

//Checks if load has diminished
bool LoadLoss() {
  if (CheckLoadVariation()) {
    if (scale.get_units() < actualLoad) {
      return true;
    }
  }
  return false;
}

//Evaluate the time to activate the motor in milliseconds between timeMin and timeMax according load and pre-set value mismatch
int EvaluateShake() {
  int timeMax = 3000;
  int timeMin = 1000;
  float t;
  t = (wrongLoad / cardWeights[cardDetected][1]);
  if (t < 1) {
    t = 1 - t;
  } else {
    t = t - 1;
  }
  t = timeMin + (timeMax - timeMin) * t;
  return round(t);
}

//FSM Start state
void Start() {
  FireAlarm(5);
  delay(1000);
  actualLoad = scale.get_units(10);
  cardDetected = 0;
}

//FSM Normal state
void Normal() {
  FireAlarm(1);
  if (CheckLoadVariation()) {
    int tmp = CheckForCard();
    if (tmp != 0) {
      cardDetected = tmp;
      state = loading;
    } else {
      state = error;
    }
  }
}

//FSM Loading state
void Loading() {
  long countDown = 8000;
  long startTime = millis();
  float load;
  bool failFlag = true;
  long 

//Players have countDown time to load the base
  while (t < startTime + countDown) {
    t = millis();
    if (LoadLoss()) {
      failFlag = true;
      break;
    }
    if (startTime + countDown - 3000 < t) {
      FireAlarm(2);
    }
  }

//Checks if there was a load loss, if the weight loaded is wrong or if everything is fine
  if (failFlag) {
    state = lost;  
  } else {
    if (CheckLoad(actualLoad + cardWeights[cardDetected][0] + cardWeights[cardDetected][1])) {
      actualLoad = scale.get_units(10);
      cardDetected = 0;
      state = normal;
    } else {
      wrongLoad =  scale.get_units() - actualLoad;
      errorFlag = true;
      state = error;
    } 
  }
}

//FSM Error state
void Error(bool type) { //type: false generic error, true loading error
  int t;
  FireAlarm(4);
  if (type) {
    t = EvaluateShake();
  } else {
    t = 250;
  }
  Shake(t);
  if (LoadLoss()) {
    state = lost;
  } else {
    state = normal;
    actualLoad = scale.get_units(10);
  }
}

//FSM Lost state
void Lost() {
// loop until base is unloaded
  while (actualLoad - tolerance > 0) {
    actualLoad = scale.get_units();
    FireAlarm(3);
    Shake(3000);
    delay(2000);
  }
  state = start; //FSM begins from scratch
}

void setup() {
  pinMode(dc1, OUTPUT);
  pinMode(dc2, OUTPUT);
  #ifdef POTENTIOMETERS
    pinMode(pot1, INPUT);
    pinMode(pot2, INPUT);
    pinMode(pot3, INPUT);
  #endif
  pinMode(led, OUTPUT);
  RapidBlink(); //led test
  Wire.begin(); // master
  analogWrite(dc2, 0); //setting motor direction

//scale reset routine
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  scale.set_scale(calibration_factor);
  
  errorFlag = false;
  state = start;
}

void loop() {
  switch (state) {
    case start:
      Start();
      break;
    case normal:
      Normal();
      break;
    case error:
      Error(errorFlag);
      errorFlag = false;
      break;
    case loading:
      Loading();
      break;
    case lost:
      Lost();
      break;
    default:
      break;
  }
}
