
#include <LiquidCrystal.h>

// ---------- LCD ----------
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 8, 13, A2, A3, A4);

// ---------- EXISTING COMPONENTS ----------
int leds[] = {2, 3, 4, 5, 6};

int motor = 9;
int ldr = A0;
int tempSensor = A1;

int button = 10;
int buzzer = 11;
int emergencyLED = 12;

// ---------- SAFETY TIMER ----------
unsigned long motorStartTime = 0;
bool motorRunning = false;
bool safetyLock = false;

const unsigned long SAFETY_TIME = 30000;   // 30 sec
const unsigned long WARNING_TIME = 25000;  // 25 sec

void setup() {

  // LEDs
  for (int i = 0; i < 5; i++) {
    pinMode(leds[i], OUTPUT);
  }

  pinMode(motor, OUTPUT);
  pinMode(button, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(emergencyLED, OUTPUT);

  Serial.begin(9600);

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();
}

void loop() {

  // =====================================
  // LDR - AUTOMATIC LIGHT CONTROL
  // =====================================

  int lightValue = analogRead(ldr);

  int number;

  if (lightValue < 200)
    number = 5;
  else if (lightValue < 400)
    number = 4;
  else if (lightValue < 600)
    number = 3;
  else if (lightValue < 800)
    number = 2;
  else
    number = 1;

  for (int i = 0; i < 5; i++) {
    if (i < number)
      digitalWrite(leds[i], HIGH);
    else
      digitalWrite(leds[i], LOW);
  }


  // =====================================
  // TMP36 TEMPERATURE
  // =====================================

  int sensorValue = analogRead(tempSensor);

  float voltage = sensorValue * (5.0 / 1023.0);
  float temperature = (voltage - 0.5) * 100.0;


  // =====================================
  // SERIAL COMMAND
  // =====================================

  int command = -1;

  if (Serial.available() > 0) {

    command = Serial.parseInt();

    // 0 = Motor OFF / reset safety timer
    if (command == 0) {

      digitalWrite(motor, LOW);

      motorRunning = false;
      safetyLock = false;

      digitalWrite(buzzer, LOW);
      digitalWrite(emergencyLED, LOW);
    }


    // 5 = Motor ON
    if (command == 5 && !safetyLock && !motorRunning) {

      digitalWrite(motor, HIGH);

      motorRunning = true;
      motorStartTime = millis();
    }
  }


  // =====================================
  // AUTOMATIC FAN BY TEMPERATURE
  // =====================================

  if (temperature >= 30 &&
      !safetyLock &&
      !motorRunning) {

    digitalWrite(motor, HIGH);

    motorRunning = true;
    motorStartTime = millis();
  }


  // =====================================
  // SAFETY TIMER
  // =====================================

  if (motorRunning) {

    unsigned long elapsedTime =
      millis() - motorStartTime;


    // Warning after 25 seconds
    if (elapsedTime >= WARNING_TIME &&
        elapsedTime < SAFETY_TIME) {

      digitalWrite(buzzer, HIGH);
      digitalWrite(emergencyLED, HIGH);
    }


    // Automatic OFF after 30 seconds
    if (elapsedTime >= SAFETY_TIME) {

      digitalWrite(motor, LOW);

      motorRunning = false;
      safetyLock = true;

      digitalWrite(buzzer, LOW);
      digitalWrite(emergencyLED, LOW);
    }
  }


  // =====================================
  // EMERGENCY BUTTON
  // =====================================

  if (digitalRead(button) == HIGH) {

    digitalWrite(buzzer, HIGH);
    digitalWrite(emergencyLED, HIGH);
  }


  // =====================================
  // LCD DISPLAY
  // =====================================

  lcd.clear();

  // Emergency has highest priority
  if (digitalRead(button) == HIGH) {

    lcd.setCursor(0, 0);
    lcd.print("!! EMERGENCY !!");

    lcd.setCursor(0, 1);
    lcd.print("HELP REQUIRED");
  }

  // Safety warning
  else if (motorRunning &&
           (millis() - motorStartTime >= WARNING_TIME)) {

    lcd.setCursor(0, 0);
    lcd.print("WARNING!");

    lcd.setCursor(0, 1);
    lcd.print("Fan Auto-Off");
  }

  // Normal display
  else {

    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0, 1);

    if (motorRunning)
      lcd.print("Fan: ON ");
    else
      lcd.print("Fan: OFF");

    lcd.print(" L:");
    lcd.print(number);
  }


  // =====================================
  // SERIAL MONITOR
  // =====================================

  Serial.print("LDR: ");
  Serial.print(lightValue);

  Serial.print(" | Temp: ");
  Serial.print(temperature);

  Serial.print(" C | Fan: ");

  if (motorRunning)
    Serial.println("ON");
  else
    Serial.println("OFF");


  delay(500);
}
