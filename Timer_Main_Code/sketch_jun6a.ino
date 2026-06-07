#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

// === PIN SETUP ===
const int onstatusLed = 8;
const int button1Pin = 5;
const int button2Pin = 7;
const int continuityPin = 12;
const int led1Pin = 4;
const int led2Pin = 10;
const int buzzerPin = 9;
const int servoPin = 13;
const int timerLedPin = 6; // LED timer / lancement

const int raspistatus = 11;

// === COMPOSANTS ===
RTC_DS1307 rtc;
Servo myServo;

// === ÉTAT ===
bool isClosed = true;
bool lastButton1State = false;
bool lastButton2State = false;
bool lastContinuityState = false;
bool rtcAvailable = false;

void setup() {
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(continuityPin, INPUT_PULLUP);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(onstatusLed, OUTPUT);
  digitalWrite(onstatusLed, HIGH);
  pinMode(timerLedPin, OUTPUT);
  digitalWrite(timerLedPin, LOW);

  pinMode(raspistatus, INPUT_PULLUP);

  myServo.attach(servoPin);
  Wire.begin();

  testBuzzer();

  rtcAvailable = rtc.begin();
  if (!rtcAvailable) {
    errorBip(3); // RTC not found
  }

  // Initial position
  closeServo();
}

void loop() {
  // === BOUTON 1 ===
  bool button1State = !digitalRead(button1Pin);
  if (button1State && !lastButton1State) {
    isClosed = !isClosed;
    if (isClosed) {
      closeServo();
      bip(2, 200, 100);
    } else {
      openServo();
      bip(1, 200, 100);
    }
  }
  lastButton1State = button1State;

// === CONTINUITÉ ===
bool continuity = !digitalRead(continuityPin);

if (continuity != lastContinuityState) {
  if (continuity) {
    // Jack branché : bip montant + clignotement LED allumage

    digitalWrite(onstatusLed, LOW);
    tone(buzzerPin, 400, 150);
    delay(150);

    digitalWrite(onstatusLed, HIGH);
    delay(100);

    digitalWrite(onstatusLed, LOW);
    tone(buzzerPin, 1000, 150);
    delay(150);

    digitalWrite(onstatusLed, HIGH);
  } else {
    // Jack débranché : bip descendant + clignotement LED allumage

    digitalWrite(onstatusLed, LOW);
    tone(buzzerPin, 1000, 150);
    delay(150);

    digitalWrite(onstatusLed, HIGH);
    delay(100);

    digitalWrite(onstatusLed, LOW);
    tone(buzzerPin, 400, 150);
    delay(150);

    digitalWrite(onstatusLed, HIGH);
  }
}

  lastContinuityState = continuity;

  // === BOUTON 2 ===
  bool button2State = !digitalRead(button2Pin);
  if (button2State && !lastButton2State) {
    handleButton2();
  }
  lastButton2State = button2State;
}

// === FONCTIONS ===

void openServo() {
  myServo.write(0); // Position ouverte
  digitalWrite(led1Pin, LOW);
}

void closeServo() {
  myServo.write(90); // Position fermée
  digitalWrite(led1Pin, HIGH);
}

void bip(int count, int duration, int gap) {
  for (int i = 0; i < count; i++) {
    tone(buzzerPin, 1000, duration);
    delay(duration + gap);
  }
}

void blinkLed(int pin, int count, int duration, int gap) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
    delay(gap);
  }
}

void errorBip(int count) {
  bip(count, 100, 100);
  blinkLed(led2Pin, count, 100, 100);
}

void handleButton2() {
  bool continuity = !digitalRead(continuityPin);
  bool multipleErrors = false;

  if (continuity && isClosed) {
    // Cas OK : test prêt
    digitalWrite(led2Pin, HIGH);

    // Mode attente de lancement :
    // tant que le câble est connecté, bip lent + LED D6
    while (digitalRead(continuityPin) == LOW) {
      digitalWrite(timerLedPin, HIGH);
      tone(buzzerPin, 800, 100);
      delay(100);
      digitalWrite(timerLedPin, LOW);
      delay(900);
    }

    // Câble déconnecté → début du chrono final
    unsigned long startMillis = millis();
    unsigned long waitTime = 7000;

    // Mode vol / timer final :
    // bip plus rapide + LED D6
    // annulation possible si bouton timer appuyé ET jack rebranché
    while (millis() - startMillis < waitTime) {

      // === ANNULATION TIMER FINAL ===
      bool buttonTimerPressed = !digitalRead(button2Pin);
      bool jackReconnected = !digitalRead(continuityPin);

      if (buttonTimerPressed && jackReconnected) {
        noTone(buzzerPin);
        digitalWrite(timerLedPin, LOW);
        digitalWrite(led2Pin, LOW);

        // Bip d'annulation
        tone(buzzerPin, 400, 200);
        delay(250);
        tone(buzzerPin, 400, 200);
        delay(250);
        noTone(buzzerPin);

        return; // on quitte sans ouvrir le servo
      }

      digitalWrite(timerLedPin, HIGH);
      tone(buzzerPin, 1000, 100);
      delay(100);
      digitalWrite(timerLedPin, LOW);
      delay(400);
    }

    // Timer terminé → ouverture
    openServo();
    digitalWrite(led2Pin, LOW);
    digitalWrite(timerLedPin, LOW);

  } else {
    multipleErrors = (!continuity && !isClosed);
    delay(100);

    if (!continuity) {
      bip(1, 500, 100);
      blinkLed(led2Pin, 1, 500, 100);
      if (multipleErrors) delay(1000);
    }

    if (!isClosed) {
      bip(2, 100, 100);
      blinkLed(led1Pin, 2, 100, 100);
      if (multipleErrors) delay(1000);
    }

    if (!rtcAvailable) {
      errorBip(3);
    }
  }
}

void testBuzzer() {
  // BIP grave
  tone(buzzerPin, 400);
  delay(300);
  noTone(buzzerPin);
  delay(200);

  // BIP médium
  tone(buzzerPin, 800);
  delay(300);
  noTone(buzzerPin);
  delay(200);

  // BIP aigu
  tone(buzzerPin, 1200);
  delay(300);
  noTone(buzzerPin);
  delay(200);

  // Bip répétitif
  for (int i = 0; i < 3; i++) {
    tone(buzzerPin, 1000);
    delay(150);
    noTone(buzzerPin);
    delay(150);
  }
}