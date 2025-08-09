#include <Wire.h>
#include <U8g2lib.h>
#include <RTClib.h>
#include <DHT.h>

U8G2_ST7920_128X64_F_SW_SPI display(U8G2_R2, 13, 11, 10);

#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

RTC_DS3231 rtc;
DateTime now;

#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 8
int xVal, yVal;
bool swState, buttonPressed = false;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 200;

#define BUZZER_PIN 9

#define ULTRASONIC_TRIG 6
#define ULTRASONIC_ECHO 5
long lastAwayTime = 0;
bool userAway = false;
const unsigned long awayTimeout = 60000; // 1 minute after alarm to watch for return

// --- Alarm ---
int alarmHour = 22, alarmMinute = 9;
bool alarmActive = false;
bool inAlarmTimeSetting = false;
bool settingHours = true;

enum MainMenuOptions { MENU_CLOCK, MENU_ALARM, MENU_GAMES };
int mainMenuIndex = 0;
const int mainMenuItems = 3;

enum AlarmMenuOptions { ALARM_SET_TIME, ALARM_BACK };
int alarmMenuIndex = 0;
const int alarmMenuItems = 2;

enum GamesMenuOptions { GAME_MATH, GAME_DODGE, GAME_MAZE, GAME_BACK };
int gamesMenuIndex = 0;
const int gamesMenuItems = 4;

bool inMainMenu = false;
bool inAlarmMenu = false;
bool inGamesMenu = false;

float temp = 0, humidity = 0;

enum AlarmGame { NONE, MATH, DODGE, MAZE};
AlarmGame currentAlarmGame = NONE;
bool gameCompleted = false;

int mathNum1, mathNum2, mathAnswer, mathUserAnswer, mathStage = 0;

const int maxDodgeBlocks = 3;
int dodgePlayerX = 7;
int dodgeObstacleX[maxDodgeBlocks];
int dodgeObstacleY[maxDodgeBlocks];
int dodgeScore = 0;
unsigned long dodgeLastMove = 0;
const int dodgeScoreToWin = 15;

const int mazeWidth = 7;
const int mazeHeight = 7;
const char mazeMap[mazeHeight][mazeWidth+1] = {
  "#######",
  "#     #",
  "# ### #",
  "# #   #",
  "# # ###",
  "#     #",
  "#######"
};
int mazePlayerX = 1, mazePlayerY = 1;
int mazeGoalX = 5, mazeGoalY = 5;

void setup() {
  Serial.begin(9600);
  dht.begin();
  rtc.begin();
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  display.begin();
  display.setFont(u8g2_font_ncenB08_tr);

  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  randomSeed(analogRead(A2));
}

void loop() {
  now = rtc.now();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Only update if valid readings
  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    temp = t;
  }

  readJoystick();

  if (alarmActive && currentAlarmGame == NONE) {
    currentAlarmGame = static_cast<AlarmGame>(random(1, 4));
    resetGameVars(currentAlarmGame);
    gameCompleted = false;
    tone(BUZZER_PIN, 1000);
    lastAwayTime = millis();
    userAway = false;
  }

  if (alarmActive) {
    long dist = readUltrasonicDistance();
    if (!userAway && dist > 50) {
      userAway = true;
      lastAwayTime = millis();
    }
    if (userAway && dist < 30 && millis() - lastAwayTime < awayTimeout) {
      currentAlarmGame = NONE;
      alarmActive = false;
      noTone(BUZZER_PIN);
      delay(200);
      alarmActive = true;
      return;
    }
  }

  if (alarmActive && !gameCompleted) {
    playAlarmGame();
  } else if (currentAlarmGame != NONE && gameCompleted) {
    noTone(BUZZER_PIN);
    alarmActive = false;
    currentAlarmGame = NONE;
    inGamesMenu = false;
    inAlarmTimeSetting = false;
    inAlarmMenu = false;
    inMainMenu = true;
  } else {
    if (inMainMenu) drawMainMenu();
    else if (inAlarmMenu) drawAlarmMenu();
    else if (inGamesMenu) {
      if (currentAlarmGame != NONE) {
        playAlarmGame();
      } else {
        drawGamesMenu();
      }
    }
    else if (mainMenuIndex == MENU_CLOCK) drawClockScreen();
    else if (mainMenuIndex == MENU_ALARM) drawAlarmClockScreen();
    else if (mainMenuIndex == MENU_GAMES) {
      display.clearBuffer();
      display.drawStr(0, 12, "Press to open");
      display.drawStr(0, 28, "Games menu");
      display.sendBuffer();
    }
  }

  checkAlarm();

  delay(100);
}

long readUltrasonicDistance() {
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  long distanceCm = duration / 29 / 2;
  if (distanceCm == 0) distanceCm = 255;
  return distanceCm;
}

void readJoystick() {
  xVal = analogRead(JOY_X);
  yVal = analogRead(JOY_Y);
  swState = !digitalRead(JOY_SW);
  unsigned long nowMs = millis();

  if (swState && !buttonPressed && nowMs - lastButtonTime > debounceDelay) {
    lastButtonTime = nowMs;
    buttonPressed = true;

    if (alarmActive && !gameCompleted && currentAlarmGame != NONE) {
      return;
    }

    if (inAlarmTimeSetting) {
      if (settingHours) settingHours = false;
      else {
        inAlarmTimeSetting = false;
        inAlarmMenu = true;
      }
      return;
    }

    if (inGamesMenu) {
      if (gamesMenuIndex == GAME_BACK) {
        inGamesMenu = false;
        inMainMenu = true;
      } else {
        currentAlarmGame = static_cast<AlarmGame>(gamesMenuIndex + 1);
        resetGameVars(currentAlarmGame);
        gameCompleted = false;
      }
      return;
    }

    if (inAlarmMenu) {
      if (alarmMenuIndex == ALARM_SET_TIME) {
        inAlarmTimeSetting = true;
        settingHours = true;
      } else if (alarmMenuIndex == ALARM_BACK) {
        inAlarmMenu = false;
        inMainMenu = true;
      }
      return;
    }

    if (inMainMenu) {
      switch(mainMenuIndex) {
        case MENU_CLOCK: inMainMenu = false; break;
        case MENU_ALARM: inMainMenu = false; inAlarmMenu = true; break;
        case MENU_GAMES: inMainMenu = false; inGamesMenu = true; break;
      }
      return;
    }

    inMainMenu = true;
  }
  if (!swState) buttonPressed = false;

  if (nowMs - lastButtonTime > debounceDelay) {
    if (inMainMenu) {
      if (yVal > 700 && mainMenuIndex > 0) mainMenuIndex--;
      else if (yVal < 300 && mainMenuIndex < mainMenuItems - 1) mainMenuIndex++;
      lastButtonTime = nowMs;
    }
    else if (inAlarmMenu) {
      if (yVal > 700 && alarmMenuIndex > 0) alarmMenuIndex--;
      else if (yVal < 300 && alarmMenuIndex < alarmMenuItems - 1) alarmMenuIndex++;
      lastButtonTime = nowMs;
    }
    else if (inGamesMenu && currentAlarmGame == NONE) {
      if (yVal > 700 && gamesMenuIndex > 0) gamesMenuIndex--;
      else if (yVal < 300 && gamesMenuIndex < gamesMenuItems - 1) gamesMenuIndex++;
      lastButtonTime = nowMs;
    }

    if (inAlarmTimeSetting) {
      if (xVal > 700) {
        if (settingHours) alarmHour = (alarmHour + 1) % 24;
        else alarmMinute = (alarmMinute + 1) % 60;
        delay(200);
      }
      else if (xVal < 300) {
        if (settingHours) alarmHour = (alarmHour == 0) ? 23 : alarmHour - 1;
        else alarmMinute = (alarmMinute == 0) ? 59 : alarmMinute - 1;
        delay(200);
      }
    }
  }
}

void checkAlarm() {
  if (now.hour() == alarmHour && now.minute() == alarmMinute && now.second() == 0 && !alarmActive) {
    alarmActive = true;
    currentAlarmGame = NONE;
  }
  if (!alarmActive) {
    noTone(BUZZER_PIN);
  }
}

void drawMainMenu() {
  display.clearBuffer();
  display.drawStr(0, 10, mainMenuIndex == MENU_CLOCK ? "> Clock" : "  Clock");
  display.drawStr(0, 20, mainMenuIndex == MENU_ALARM ? "> Alarm" : "  Alarm");
  display.drawStr(0, 30, mainMenuIndex == MENU_GAMES ? "> Games" : "  Games");
  display.sendBuffer();
}

void drawAlarmMenu() {
  display.clearBuffer();
  display.drawStr(0, 10, "Alarm Menu");
  display.drawStr(0, 20, alarmMenuIndex == ALARM_SET_TIME ? "> Set Alarm Time" : "  Set Alarm Time");
  display.drawStr(0, 30, alarmMenuIndex == ALARM_BACK ? "> Back" : "  Back");
  display.sendBuffer();
}

void drawGamesMenu() {
  display.clearBuffer();
  display.drawStr(0, 10, gamesMenuIndex == GAME_MATH ? "> Math Game" : "  Math Game");
  display.drawStr(0, 20, gamesMenuIndex == GAME_DODGE ? "> Dodge Game" : "  Dodge Game");
  display.drawStr(0, 30, gamesMenuIndex == GAME_MAZE ? "> Maze Game" : "  Maze Game");
  display.drawStr(0, 50, gamesMenuIndex == GAME_BACK ? "> Back" : "  Back");
  display.sendBuffer();
}

void drawClockScreen() {
  display.clearBuffer();

  display.setFont(u8g2_font_logisoso32_tf);
  char timeStr[10];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d", now.hour(), now.minute());
  display.drawStr(0, 40, timeStr);

  display.setFont(u8g2_font_ncenB08_tr);
  char secondsStr[4];
  snprintf(secondsStr, sizeof(secondsStr), ":%02d", now.second());
  display.drawStr(95, 38, secondsStr);

  char dateStr[16];
  snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  display.drawStr(0, 54, dateStr);

  char tempStr[20];
  dtostrf(temp, 4, 1, tempStr);
  display.drawStr(0, 64, (String("Temp: ") + tempStr).c_str());

  char humStr[20];
  dtostrf(humidity, 4, 1, humStr);
  display.drawStr(64, 64, (String("Hum: ") + humStr).c_str());

  display.sendBuffer();
}

void drawAlarmClockScreen() {
  display.clearBuffer();
  display.drawStr(0, 12, "Alarm Clock");

  char alarmStr[20];
  snprintf(alarmStr, sizeof(alarmStr), "Alarm: %02d:%02d", alarmHour, alarmMinute);
  display.drawStr(0, 28, alarmStr);

  if (alarmActive) {
    display.drawStr(0, 44, "ALARM RINGING!");
    display.drawStr(0, 60, "Solve game to stop");
  } else if (inAlarmTimeSetting) {
    display.drawStr(0, 44, settingHours ? "Set HOUR" : "Set MIN");
    display.drawStr(0, 60, "Press to next");
  } else {
    display.drawStr(0, 44, alarmMenuIndex == ALARM_SET_TIME ? "> Set Alarm Time" : "  Set Alarm Time");
    display.drawStr(0, 60, alarmMenuIndex == ALARM_BACK ? "> Back" : "  Back");
  }

  display.sendBuffer();
}


void resetGameVars(AlarmGame game) {
  switch (game) {
    case MATH:
      mathNum1 = random(1, 10);
      mathNum2 = random(1, 10);
      mathAnswer = mathNum1 + mathNum2;
      mathUserAnswer = 0;
      mathStage = 0;
      break;

    case DODGE:
      dodgePlayerX = 7;
      dodgeScore = 0;
      dodgeLastMove = millis();
      for (int i = 0; i < maxDodgeBlocks; i++) {
        dodgeObstacleX[i] = random(0, 15);
        dodgeObstacleY[i] = random(-8, 0);
      }
      break;

    case MAZE:
      mazePlayerX = 1;
      mazePlayerY = 1;
      gameCompleted = false;
      break;

    default:
      break;
  }
}

void playAlarmGame() {
  switch (currentAlarmGame) {
    case MATH: playMathGame(); break;
    case DODGE: playDodgeGame(); break;
    case MAZE: playMazeGame(); break;
    default: break;
  }
}

void playMathGame() {
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  char buf[32];
  snprintf(buf, sizeof(buf), "Solve: %d + %d =", mathNum1, mathNum2);
  display.drawStr(0, 12, buf);

  snprintf(buf, sizeof(buf), "Ans: %d", mathUserAnswer);
  display.drawStr(0, 28, buf);
  display.drawStr(0, 44, "Press Btn to submit");

  display.sendBuffer();

  if (xVal > 700) {
    mathUserAnswer++;
    delay(150);
  } else if (xVal < 300 && mathUserAnswer > 0) {
    mathUserAnswer--;
    delay(150);
  }

  if (swState && !buttonPressed) {
    buttonPressed = true;
    if (mathUserAnswer == mathAnswer) {
      gameCompleted = true;
    }
  }
  if (!swState) buttonPressed = false;
}

void playDodgeGame() {
  display.clearBuffer();
  display.drawStr(0, 0, "Dodge blocks!");
  display.drawFrame(0, 12, 128, 48);

  unsigned long nowMs = millis();

  display.drawBox(dodgePlayerX * 8, 56, 8, 8);

  for (int i = 0; i < maxDodgeBlocks; i++) {
    display.drawBox(dodgeObstacleX[i] * 8, dodgeObstacleY[i] * 8 + 12, 8, 8);
  }

  display.setCursor(0, 63);
  display.print("Score: ");
  display.print(dodgeScore);

  display.sendBuffer();

  if (xVal > 700 && dodgePlayerX < 15) {
    dodgePlayerX++;
    delay(150);
  } else if (xVal < 300 && dodgePlayerX > 0) {
    dodgePlayerX--;
    delay(150);
  }

  if (nowMs - dodgeLastMove > 400) {
    dodgeLastMove = nowMs;
    for (int i = 0; i < maxDodgeBlocks; i++) {
      dodgeObstacleY[i]++;
      if (dodgeObstacleY[i] > 5) {
        if (dodgeObstacleX[i] == dodgePlayerX) {
          // Hit! Fail game & stop alarm
          gameCompleted = false;
          alarmActive = false;
          noTone(BUZZER_PIN);
          return;
        } else {
          dodgeScore++;
          dodgeObstacleX[i] = random(0, 15);
          dodgeObstacleY[i] = 0;
          if (dodgeScore >= dodgeScoreToWin) {
            gameCompleted = true;
            return;
          }
        }
      }
    }
  }

  if (swState && !buttonPressed) {
    buttonPressed = true;
  }
  if (!swState) buttonPressed = false;
}

void playMazeGame() {
  display.clearBuffer();

  for (int y = 0; y < mazeHeight; y++) {
    for (int x = 0; x < mazeWidth; x++) {
      int px = x * 8;
      int py = 10 + y * 8;

      if (mazeMap[y][x] == '#') display.drawBox(px, py, 8, 8);
      else display.drawFrame(px, py, 8, 8);

      if (x == mazePlayerX && y == mazePlayerY) display.drawStr(px + 2, py + 7, "P");
      if (x == mazeGoalX && y == mazeGoalY) display.drawStr(px + 2, py + 7, "G");
    }
  }
  display.sendBuffer();

  if (xVal > 700 && canMove(mazePlayerX + 1, mazePlayerY)) {
    mazePlayerX++;
    delay(300);
  } else if (xVal < 300 && canMove(mazePlayerX - 1, mazePlayerY)) {
    mazePlayerX--;
    delay(300);
  } else if (yVal > 700 && canMove(mazePlayerX, mazePlayerY - 1)) {
    mazePlayerY--;
    delay(300);
  } else if (yVal < 300 && canMove(mazePlayerX, mazePlayerY + 1)) {
    mazePlayerY++;
    delay(300);
  }

  if (mazePlayerX == mazeGoalX && mazePlayerY == mazeGoalY) {
    gameCompleted = true;
  }
}

bool canMove(int x, int y) {
  if (x < 0 || x >= mazeWidth || y < 0 || y >= mazeHeight) return false;
  return mazeMap[y][x] != '#';
}