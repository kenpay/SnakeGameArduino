#include <LedControl.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#define joyStickXPin A0
#define joyStickYPin A1
#define joyStickButtonPin 13
#define joyStickDefaultUpperValue 768
#define joyStickDefaultLowerValue 256
#define leftDirection 0
#define rightDirection 1
#define downDirection 2
#define upDirection 3
#define noDirection 4
#define lcdV0Pin 9

byte screen, prevScreen, swipeDirection, snakeDirection, difficulty, prevSnakeTailX, prevSnakeTailY, snakeLength, foodX, foodY, index, shouldEat[64], shouldEatLength, foodStackX[64], foodStackY[64];
short joyStickXValue, joyStickYValue, snakeHeadX[64], snakeHeadY[64], highScore, score;
unsigned long long lastUpdate;
bool started;
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); 
LedControl lc(10, 11, 12);

void changeScreen(byte nextScreen)
{
  lcd.clear();
  if (nextScreen == 0)
  { 
    lcd.print("Press joystick");
    lcd.setCursor(0,1);
    lcd.print("button to START!");
  }
  else if (nextScreen == 1)
  {
    lcd.print("Highscore:");
    lcd.setCursor(0,1);
    lcd.print(highScore);
  }
  else if (nextScreen == 2)
  {
    lcd.print("Choose difficulty:");
    lcd.setCursor(0,1);
    lcd.print("1.Easy 2.Med 3.Hard");
  }
  else
  {
    lcd.print("Score:");
    lcd.setCursor(0,1);
    lcd.print(score);
  }
  prevScreen = nextScreen;
  lc.clearDisplay(0);
  delay(250);
}

void getHighScore()
{
  short address = 0;
  byte value;
  do{
    value = EEPROM.read(address++);
    highScore+= value;
  }while(value);
}

void setup() {
  Serial.begin(9600);
  pinMode(joyStickButtonPin, INPUT);
  digitalWrite(joyStickButtonPin, HIGH);
  pinMode(lcdV0Pin, OUTPUT);
  analogWrite(lcdV0Pin, 90);
  getHighScore();
  lcd.begin(16,2);
  changeScreen(0);
  lc.shutdown(0, false);
  lc.setIntensity(0,2);
  lc.clearDisplay(0);
}

byte getSwipeDirection()
{
  joyStickXValue = analogRead(joyStickXPin), joyStickYValue = analogRead(joyStickYPin);
  if (joyStickXValue>joyStickDefaultUpperValue)
    return rightDirection;
  if (joyStickXValue<joyStickDefaultLowerValue)
    return leftDirection;
  if (joyStickYValue>joyStickDefaultUpperValue)
    return upDirection;
  if (joyStickYValue<joyStickDefaultLowerValue)
    return downDirection;
  if (started)
    return swipeDirection;
  return noDirection;
}

void getSnakeDirection()
{
  if (swipeDirection == rightDirection && snakeDirection != leftDirection)
    snakeDirection = rightDirection;
  if (swipeDirection == leftDirection && snakeDirection != rightDirection)
    snakeDirection = leftDirection;
  if (swipeDirection == upDirection && snakeDirection != downDirection)
    snakeDirection = upDirection;
  if (swipeDirection == downDirection && snakeDirection != upDirection)
    snakeDirection = downDirection;
}

void moveSnake()
{
  if (snakeDirection == downDirection)
    snakeHeadY[0]++;
  else if (snakeDirection == upDirection)
    snakeHeadY[0]--;
  else if (snakeDirection == leftDirection)
    snakeHeadX[0]--;
  else
    snakeHeadX[0]++;
  if (snakeHeadX[0] < 0)
    snakeHeadX[0] = 7;
  if (snakeHeadX[0] >7)
    snakeHeadX[0] = 0;
  if (snakeHeadY[0] < 0)
    snakeHeadY[0] = 7;
  if (snakeHeadY[0] >7)
    snakeHeadY[0] = 0;
}

void setDifficultyLeds(bool difficultyBool)
{
  for (index=0;index<=difficulty;++index)
  {
    lc.setLed(0, index, index, difficultyBool);
    lc.setLed(0, 7-index, 7-index, difficultyBool);
    lc.setLed(0, 7-index, index, difficultyBool);
    lc.setLed(0, index, 7-index, difficultyBool);
  }
}

void flashDifficulty()
{
  if (millis()-lastUpdate>=250)
  {
    setDifficultyLeds(false);
    if (swipeDirection == leftDirection && difficulty)
        difficulty--;
    else if(swipeDirection == rightDirection && difficulty<2)
        difficulty++;
    delay(200);
    setDifficultyLeds(true);
    lastUpdate = millis();
  }
}

bool cannotGenerateFood()
{
  for (index=0;index<=snakeLength;index++)
    if (snakeHeadX[index] == foodX && snakeHeadY[index] == foodY)
      return false;
  return true;
}

bool collision()
{
  for (index=2;index<=snakeLength;index++)
    if (snakeHeadX[index] == snakeHeadX[0] && snakeHeadY[index] == snakeHeadY[0])
      return true;
  return false;
}

void updateSnake()
{
  for (index=snakeLength;index>=1;--index)
  {
    snakeHeadX[index]=snakeHeadX[index-1];
    snakeHeadY[index]=snakeHeadY[index-1];
  }
}

void generateFood()
{
  do
  {
    foodX = random(8);
    foodY = random(8);
  }
  while(!cannotGenerateFood());
  lc.setLed(0, foodX, foodY, 1);
}

void updateHighscore()
{
  highScore = score;
  short address = 0;
  byte value;
  do{
    if (score > 255)
      value = 255, score-=255;
    else
      if (score==value)
        value=0;
      else
        value = score;
    EEPROM.write(address++, value);
  }while(value);
}

void resetGame()
{
  started = false;
  if (snakeLength == 63)
    updateScore();
  delay(2500);
  screen = 0;
  changeScreen(0);
  snakeLength = 0;
  shouldEat[0] = 0;
  shouldEatLength = 0;
  if (score > highScore)
    updateHighscore();
  score = 0;
}

void updateScore()
{
  shouldEat[shouldEatLength] = snakeLength+1;
  foodStackX[shouldEatLength] = foodX;
  foodStackY[shouldEatLength++] = foodY;
  generateFood();
  score += 3+difficulty;
  lcd.setCursor(0,1);
  lcd.print(score);
}

void increaseEatingTiming()
{
  byte index;
  for (index=snakeLength; index<shouldEatLength;index++)
    shouldEat[index]++;
}

void loop() {
  swipeDirection = getSwipeDirection();
  if(!started)
  {
    started = !digitalRead(joyStickButtonPin);
    if (screen==2)
    {
      flashDifficulty();
      if (started)
        changeScreen(3), generateFood();
    }
    else
    {
      if (swipeDirection == rightDirection && screen == 0)
        screen++;
      if (swipeDirection == leftDirection && screen == 1)
        screen--;
      if (started)
        screen = 2, started=false;
      if (prevScreen != screen)
        changeScreen(screen);
    }
  }
  else
  {
    if (millis()-lastUpdate >= 500-(2*snakeLength/3-difficulty))
    {
      getSnakeDirection();
      updateSnake();
      moveSnake();
      if (shouldEat[snakeLength] != 1)
        lc.setLed(0, prevSnakeTailX, prevSnakeTailY, 0);
      for (index=snakeLength; index<shouldEatLength;index++)
      if (shouldEat[index])
      {
        shouldEat[index]--;
        if (!shouldEat[index])
        {
          snakeHeadX[++snakeLength] = foodStackX[index];
          snakeHeadY[snakeLength] = foodStackY[index];
          increaseEatingTiming();
        }
      }
      if (snakeHeadX[0] == foodX && snakeHeadY[0] == foodY)
        updateScore();
      else
        lc.setLed(0, snakeHeadX[0], snakeHeadY[0], 1);
      prevSnakeTailX = snakeHeadX[snakeLength];
      prevSnakeTailY = snakeHeadY[snakeLength];
      if (collision() || snakeLength == 63)
        resetGame();
      lastUpdate = millis();
    }
  }
}
