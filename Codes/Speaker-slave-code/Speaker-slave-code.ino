#include <TinyWireS.h>

#define led 4
#define speaker 1
#define highTone 255
#define midHighTone 189
#define midTone 125
#define midLowTone 65
#define midLowLowTone 33
#define lowTone 1

#define lapse 5000 //time refactored by 12
#define shortLapse 10

int t = 1;

//clock refactored x12

void RapidBlink() {
    digitalWrite(led, HIGH);
    delay(shortLapse);
    digitalWrite(led, LOW);
    delay(shortLapse);
    digitalWrite(led, HIGH);
    delay(shortLapse);
    digitalWrite(led, LOW);
    delay(shortLapse);
}

void Blink(int n) {
  for (int k = 0; k < n; k++) {
    digitalWrite(led, HIGH);
    delay(lapse); 
    digitalWrite(led, LOW);
    delay(lapse);
  }
}

void RapidBuzz() {
  analogWrite(speaker, lowTone);
  delay(shortLapse * 5);
  analogWrite(speaker, 0);
  delay(shortLapse);
  analogWrite(speaker, lowTone);
  delay(shortLapse * 5);
  analogWrite(speaker, 0);
}

void Alarm() {
  TinyWireS.write(1); //sends a byte to master
  switch (t) {
    case 1: //normal
      Buzz();
    break;
    case 2: //urgent
      UrgentBuzz();
    break;
    case 3: //fail
      FailBuzz();
    break;
    case 4: //error
      ErrorBuzz();
    break;
    case 5: //restart
      RestartBuzz();
    break;
    default:
    break;
    }
  Blink(1);
  }
    
void SetAlarm(int x) {
  while (1 < TinyWireS.available()) {
    t = TinyWireS.read();
  }
}

void setup() {
  pinMode(led, OUTPUT);
  pinMode(speaker, OUTPUT);
  RapidBlink();
  RapidBuzz();
  
  TinyWireS.begin(1); //slave address 1
  TinyWireS.onRequest(Alarm);
  TinyWireS.onReceive(SetAlarm);
}

void FailBuzz() {
  analogWrite(speaker, midLowTone);
  delay(lapse * 2);
  analogWrite(speaker, 0);
  analogWrite(speaker, lowTone);
  delay(lapse * 4);
  analogWrite(speaker, 0);
  }

void Buzz() {
  analogWrite(speaker, lowTone);
  delay(lapse);
  analogWrite(speaker, 0);
  delay(lapse);
}

void ErrorBuzz() {
  for (int i = 0; i < 3; i++) {
  analogWrite(speaker, lowTone);
  delay(lapse/3);
  analogWrite(speaker, 0);
  delay(lapse/4);
  }
}

void UrgentBuzz() {
  analogWrite(speaker, midLowTone);
  delay(lapse/2);
  analogWrite(speaker, 0);
  delay(lapse/2);
  }

void RestartBuzz() {
  analogWrite(speaker, lowTone);
  delay(lapse/2);
  analogWrite(speaker, 0);
  analogWrite(speaker, midTone);
  delay(lapse/2);
  analogWrite(speaker, 0);
  analogWrite(speaker, midHighTone);
  delay(lapse/2);
  analogWrite(speaker, 0);
  }
  
void loop() {
}
