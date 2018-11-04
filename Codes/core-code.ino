#include "HX711.h"
//#include "Wire.h"

//#define POTENTIOMETERS //for future implementations

//motor pins
#define dc1 5
#define dc2 6
#define dcEnable 9

//load bar pins
#define DOUT  3
#define CLK  2

#define genericDelay 500
#define shortDelay 250
#define longDelay 1000

#ifdef POTENTIOMETERS
  #define pot1 A0
  #define pot2 A1
  #define pot3 A2
#endif

//pre-weighted values of cards and associated blocks
//calculate and modify according your project
#define cards 7
const int cardWeights[cards][2] = {
  {0, 0}, //placeholder
  {7, 9}, //1
  {10, 6}, //2
  {13, 36}, //3
  {18, 39}, //4
  {23, 11}, //5
  {34, 24} //6
};

//FSM state variable
enum State {idle, started, normal, error, loading, lost, won};
State state;

float actualLoad;
float wrongLoad;
float tolerance = 1.5; //grams of load measurement tolerance
int loadAssessment = 500; //ms of loading assessment
int cardDetected;

HX711 scale(DOUT, CLK);
float calibrationFactor = 210000;
bool errorFlag;

//Motor shakes t milliseconds
void Shake(int t) {
  Serial.print(t);
  Serial.println(" ms");
  analogWrite(dc1, 255);
  delay(t);
  analogWrite(dc1, 0);
}

/* Momentarily disabled
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
*/

//Compares actual load against given value
bool CheckLoad(int value) {
  delay(loadAssessment); //load assessment delay
  float load = scale.get_units() * 1000;
  if ((load < value + tolerance) && (load > value - tolerance))
    return true;
  return false;
}

//Checks if load has varied
bool CheckLoadVariation(){
  float tmp = scale.get_units() * 1000;
  Serial.print("Load: ");
  Serial.print(tmp);
  Serial.println(" gr");
  if ((tmp > actualLoad + tolerance) || (tmp < actualLoad - tolerance))
    return true;
  return false;
}

//Checks if a card has been loaded. Returns 0 if no card is matched
int CheckForCard() {
  for (short i = 0; i < cards; i++) {
   if (CheckLoad(cardWeights[i][0] + actualLoad) ) {
    Serial.print("Card ");
    Serial.print(i);
    Serial.println(" detected\n");
    return i; 
   }
  }
  return 0;
}

//Checks if load has diminished
bool LoadLoss() {
  if (CheckLoadVariation()) {
    //Extra-checks if variation was a substraction
    if (scale.get_units() * 1000 < actualLoad - tolerance - 6) { //6 is the less wheighing card
      Serial.println("You made something fall!\n");
      return true;
    }
  }
  return false;
}

//Evaluate the time to activate the motor in milliseconds between timeMin and timeMax according load and pre-set value mismatch
int EvaluateShake() {
  int timeMax = 3000;
  int timeMin = 1000;
  int minWeight = 0;
  int maxWeight = 80; // 80g as indicative max erroneous load 
  float t;

  t = wrongLoad - cardWeights[cardDetected][1] ;
  if (t < 0) {
      t *= -1;
  }
  t = ((timeMax - timeMin) * t) / maxWeight; 
  t = timeMin + t;
  return round(t);
}

//FSM Idle state
void Idle() {
  delay(genericDelay);
  if (CheckLoadVariation()) {
      int tmp = CheckForCard();
      if (tmp != 0) {
        cardDetected = tmp;
        state = started;
      }
    } 
  }

//FSM Start state
void Start() {
  //FireAlarm(5);
  if (cardDetected != 0) {
    state = loading;
  } else {
    state = normal;
  }
}

//FSM Normal state
void Normal() {
  delay(genericDelay);
  if (CheckLoadVariation()) {
    int tmp = CheckForCard();
    if (tmp != 0) {
      cardDetected = tmp;
      state = loading;
    } else {
        errorFlag = false;
        state = error;
    }
  }
}

//FSM Loading state
void Loading() {
  delay(genericDelay); //assessment delay
  long countDown = 8000; //seconds of load time
  long startTime = millis();
  float load;
  bool failFlag = false;
  long t;
  //Players have countDown time to load the base
  while (t < startTime + countDown) {
    t = millis();
    if (LoadLoss()) {
      failFlag = true;
      break;
    }
    /*
    if (startTime + countDown - 3000 < t) {
      FireAlarm(2);
    }*/
  }
  //Checks if there was a load loss, if the weight loaded is wrong or if everything is fine
  if (failFlag) {
    state = lost;  
  } else {
    Serial.print("Expetected value: ");
    Serial.println(actualLoad + cardWeights[cardDetected][0] + cardWeights[cardDetected][1]);
    if (CheckLoad(actualLoad + cardWeights[cardDetected][0] + cardWeights[cardDetected][1])) {
      Serial.println("Well done!\n");
      actualLoad = scale.get_units() * 1000;
      cardDetected = 0;
      state = normal;
    } else {
      Serial.println("Fasten your seatbelt.\n");
      wrongLoad = scale.get_units() * 1000 - actualLoad;
      errorFlag = true;
      state = error;
    } 
  }
}

//FSM Error state
//errorFlag: false generic error, true loading error
void Error() { 
  int t = genericDelay;
  //FireAlarm(4);
  if (errorFlag) {
    t = EvaluateShake();
  }
  Serial.print("Gonna shake for: ");
  Shake(t);
  if (LoadLoss()) {
    state = lost;
  } else {
    state = normal;
    delay(genericDelay);
    actualLoad = scale.get_units() * 1000;
    Serial.println("Actual load reset");
  }
}

//FSM Lost state
void Lost() {
  // loop until base is unloaded
  while (actualLoad - tolerance > 0) {
    actualLoad = scale.get_units() * 1000;
    //FireAlarm(3);
    Shake(longDelay);
    delay(1000);
  }
  state = idle; //FSM begins from scratch
}

void Won() {
//Pandemonium not implemented yet
}

void setup() {
  pinMode(dc1, OUTPUT);
  pinMode(dc2, OUTPUT);
  pinMode(dcEnable, OUTPUT);
  analogWrite(dc2, 0);
  analogWrite(dc1, 0);
  digitalWrite(dcEnable, HIGH);
  
  #ifdef POTENTIOMETERS
    pinMode(pot1, INPUT);
    pinMode(pot2, INPUT);
    pinMode(pot3, INPUT);
  #endif
  
  //Wire.begin(); // master
  
  //scale reset routine
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  scale.set_scale(calibrationFactor);
  
  actualLoad = scale.get_units() * 1000;
  cardDetected = 0;
  
  state = idle;

  Serial.begin(9600);
  Serial.println("Let's start");
  Shake(shortDelay);

}

void loop() {
  Serial.print("Actual state: ");
  Serial.println(State());
  switch (state) {
    case idle: 
      Idle();
      break;
    case started:
      Start();
      break;
    case normal:
      Normal();
      break;
    case error:
      Error();
      break;
    case loading:
      Loading();
      break;
    case lost:
      Lost();
      break;
    case won:
      Won();
      break;
    default:
      //Should never happen
      break;
  }
}

String State() {
  String tmp;
  switch (state) {
  case idle: 
      tmp = "idle";
      break;
    case started:
      tmp = "started";
      break;
    case normal:
      tmp = "normal";
      break;
    case error:
      tmp = "error";
      break;
    case loading:
      tmp = "loading";
      break;
    case lost:
      tmp = "lost";
      break;
    case won:
      tmp = "won";
      break;
    default:
    tmp = "meh?";
      break; 
      return tmp;
  }
}
