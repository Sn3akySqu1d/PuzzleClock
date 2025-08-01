#include <Wire.h>
#include <RTClib.h>
#include <U8g2lib.h>
#include <Encoder.h>
#include <DHT.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0);
RTC_DS3231 rtc;

#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Encoder menuEnc(2, 3);
#define ENCODER_BTN 4

#define JOY_X A0
#define JOY_Y A1
#define JOY_BTN 8

#define TRIG_PIN 5
#define ECHO_PIN 6

#define BUZZER_PIN 9

int alarmHour = 7, alarmMinute = 30;
bool alarmActive = false, puzzleSolved = false, waitingForReturn = false;
unsigned long alarmTriggerTime = 0, returnCheckWindow = 30000;

int menuIndex = 0;
String menuItems[] = {"Show Time", "Set Alarm", "Game Mode"};
const int menuSize = 3;
bool inMenu = true;

int currentGame = 0;
bool gameActive = false;
bool gameStarted = false;
bool gameFailed = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  rtc.begin();
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(JOY_BTN, INPUT_PULLUP);

  display.begin();
  display.setFont(u8g2_font_6x12_tf);
}

void loop() {
  DateTime now = rtc.now();

  if (inMenu) {
    handleMenu();
  } else {
    if (menuIndex == 0) {
      showClockScreen(now);
    } else if (menuIndex == 1) {
      setAlarmScreen();
    } else if (menuIndex == 2) {
      gameLoop();
    }
  }

  if (now.hour() == alarmHour && now.minute() == alarmMinute && !alarmActive && !puzzleSolved) {
    alarmActive = true;
    alarmTriggerTime = millis();
    currentGame = (now.day() + now.month()) % 3;
    gameActive = true;
    gameStarted = false;
    gameFailed = false;
  }

  if (alarmActive && gameActive && !puzzleSolved) {
    tone(BUZZER_PIN, 1000);

    if (!gameStarted) {
      display.clearBuffer();
      display.setCursor(0, 20);
      display.print("Alarm! Press encoder btn");
      display.setCursor(0, 40);
      display.print("to start game");
      display.sendBuffer();
      if (digitalRead(ENCODER_BTN) == LOW) {
        gameStarted = true;
        delay(300);
      }
    } else if (gameFailed) {
      display.clearBuffer();
      display.setCursor(0, 20);
      display.print("You Lost!");
      display.setCursor(0, 40);
      display.print("Press encoder btn to retry");
      display.sendBuffer();
      if (digitalRead(ENCODER_BTN) == LOW) {
        delay(300);
        currentGame = (currentGame + 1) % 3;
        gameFailed = false;
        gameStarted = false;
      }
    } else {
      if (currentGame == 0) mathsGame();
      else if (currentGame == 1) dodgeGame();
      else if (currentGame == 2) mazeGame();
    }
  }

  if (puzzleSolved && millis() - alarmTriggerTime < returnCheckWindow) {
    if (readDistance() < 50) {
      puzzleSolved = false;
      alarmActive = false;
      gameActive = false;
      gameStarted = false;
    }
  }
}

void handleMenu() {
  long pos = menuEnc.read() / 4;
  menuIndex = constrain(pos % menuSize, 0, menuSize - 1);
  display.clearBuffer();
  display.setCursor(0, 10);
  display.print("-- Menu --");
  for (int i = 0; i < menuSize; i++) {
    display.setCursor(0, 25 + i * 10);
    if (i == menuIndex) display.print("> ");
    else display.print("  ");
    display.print(menuItems[i]);
  }
  display.sendBuffer();

  if (digitalRead(ENCODER_BTN) == LOW) {
    delay(300);
    inMenu = false;
    menuEnc.write(0);
  }
}

void showClockScreen(DateTime now) {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  display.clearBuffer();
  display.setCursor(0, 12);
  display.printf("Time: %02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.setCursor(0, 24);
  display.printf("Alarm: %02d:%02d", alarmHour, alarmMinute);
  display.setCursor(0, 36);
  display.printf("Temp: %.1fC", t);
  display.setCursor(0, 48);
  display.printf("Hum: %.0f%%", h);
  display.sendBuffer();

  if (digitalRead(ENCODER_BTN) == LOW) {
    delay(300);
    inMenu = true;
  }
}

void setAlarmScreen() {
  static int sel = 0;
  long val = menuEnc.read() / 4;
  if (sel == 0) alarmHour = constrain(val % 24, 0, 23);
  else alarmMinute = constrain(val % 60, 0, 59);

  display.clearBuffer();
  display.setCursor(0, 20);
  display.print("Set Alarm Time");
  display.setCursor(0, 35);
  display.printf("%02d:%02d", alarmHour, alarmMinute);
  display.setCursor(0, 50);
  display.print(sel == 0 ? "> Hour" : "> Min");
  display.sendBuffer();

  if (digitalRead(ENCODER_BTN) == LOW) {
    delay(300);
    sel++;
    menuEnc.write(0);
    if (sel > 1) {
      sel = 0;
      inMenu = true;
    }
  }
}

void mathsGame() {
  int a = (millis() / 1000) % 10 + 1;
  int b = (millis() / 700) % 10 + 1;
  int correct = a + b;
  int input = menuEnc.read() / 4;

  display.clearBuffer();
  display.setCursor(0, 20);
  display.printf("Solve: %d + %d = ?", a, b);
  display.setCursor(0, 40);
  display.printf("Ans: %d", input);
  display.sendBuffer();

  if (digitalRead(ENCODER_BTN) == LOW && input == correct) {
    puzzleSolved = true;
    noTone(BUZZER_PIN);
  }
}

void dodgeGame() {
  static int pos = 2;
  int x = analogRead(JOY_X);
  if (x < 400 && pos > 0) pos--;
  if (x > 600 && pos < 4) pos++;
  int obstacle = (millis() / 200) % 5;
  display.clearBuffer();
  display.setCursor(0, 10);
  display.print("Avoid the X");
  for (int i = 0; i < 5; i++) {
    display.setCursor(i * 24, 30);
    if (i == obstacle) display.print("X");
    else display.print(" ");
  }
  display.setCursor(pos * 24, 50);
  display.print("O");
  display.sendBuffer();

  if (pos == obstacle) gameFailed = true;
  if (puzzleSolved) noTone(BUZZER_PIN);
}

void mazeGame() {
  static int px = 0, py = 0;
  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);
  if (x < 400 && px > 0) px--;
  if (x > 600 && px < 7) px++;
  if (y < 400 && py > 0) py--;
  if (y > 600 && py < 5) py++;
  display.clearBuffer();
  display.setCursor(0, 10);
  display.print("Exit at (7,5)");
  display.drawBox(px * 16, py * 8 + 20, 8, 8);
  display.sendBuffer();
  if (px == 7 && py == 5) {
    puzzleSolved = true;
    noTone(BUZZER_PIN);
  }
}

void gameLoop() {
  display.clearBuffer();
  display.setCursor(0, 20);
  display.print("Game Loaded");
  display.setCursor(0, 40);
  display.print("Use joystick to play");
  display.sendBuffer();
  delay(1500);
  inMenu = true;
}

int readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}