/*
Scroller Version 2.0 by jlangvand, jlangvand@gmail.com

Copyright 2013 Joakim Langvand

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


Usage:

This program is written for the Arduino with a 16x2 characters LCD on a LCD Shield.

The program could run on a 20x4 with small modifications.

Feel free to expand the program. Please send me an email if you do something cool! ;)

--
Joakim Langvand
jlangvand@gmail.com

*/

#include <TimedAction.h>    // Handling the timing of events
#include <EEPROM.h>         // Saving highscore (and settings) to EEPROM
#include <LiquidCrystal.h>  // Handling the LCD

LiquidCrystal lcd(8,9,4,5,6,7); // Initialize the LCD

#define BTN_UP 4
#define BTN_DOWN 3
#define BTN_LEFT 2
#define BTN_RIGHT 5
#define BTN_SELECT 1
#define BTN_NONE 0

#define BUZZER_PIN 12
#define DEF_FREQ 700

#define SCORE_LINE 0

#define LCD_COLS 16
#define LCD_ROWS 2

#define POS_X 0
#define POS_Y 1

#define POS_X_MIN 0
#define POS_X_MAX LCD_COLS -1
#define POS_Y_MIN 0
#define POS_Y_MAX LCD_ROWS -1

#define MAX_ENEMIES 7
#define MAX_BULLETS 4

#define NEW_ENEMY_PROBABILITY 25

#define DEBUG 1
#define ENABLE_SOUND_BY_DEFAULT 1
#define ENABLE_HOLD 1

#define CHAR_PLAYER 0
#define CHAR_ENEMY 1
#define CHAR_BULLET 2

#define VERSION "v2.0"

// Define some nice "sprites"

byte playerChar[8] = {
  B00000,
  B10000,
  B11010,
  B11111,
  B11010,
  B10000,
  B00000,
};

byte bulletChar[8] = {
  B00000,
  B00000,
  B00100,
  B01110,
  B00100,
  B00000,
  B00000,
};

byte enemyChar[8] = {
  B00001,
  B01011,
  B01111,
  B11111,
  B01111,
  B01011,
  B00001,
};

class Enemy {
  int *x, *y;
  //int x,y;
  public:
  int getx();
  int gety();
  Enemy(int);
  Enemy(int,int);
  ~Enemy();
  void moveLeft();
  void moveRight();
};

Enemy::Enemy(int sety) {
  x = new int;
  y = new int;
  *x = 15;
  *y = sety;
}

Enemy::Enemy(int setx, int sety) {
  x = new int;
  y = new int;
  *x = setx;
  *y = sety;
}

Enemy::~Enemy() {
  delete x;
  delete y;
}

void Enemy::moveLeft() { *x = *x - 1; }

void Enemy::moveRight() { *x = *x + 1; }

int Enemy::getx() { return *x; }

int Enemy::gety() { return *y; }

//int level=0;

int playerPos[2] = {0,1};

int score = 0;
int scoreDisplay = 0;
int highscore;

int buzzEnable = ENABLE_SOUND_BY_DEFAULT;

Enemy* enemy[MAX_ENEMIES];
int exist[MAX_ENEMIES];

Enemy* bullet[MAX_BULLETS];
int bullet_exists[MAX_BULLETS];

// Initialize timed actions
TimedAction checkForInput = TimedAction( 20, saveButtonState );
TimedAction drawScreen = TimedAction( 100, updateScreen );
TimedAction detectCollision = TimedAction( 50, checkCollision );
TimedAction playerLogics = TimedAction( 100, movePlayer );
TimedAction bulletLogics = TimedAction( 100, moveBullets );
TimedAction enemyLogics = TimedAction( 100, moveEnemies );

// Initialize buttons
int currentButton = 0;
int lastButton = 0;

void setup() {
  // Upload custom chars to display
  lcd.createChar( CHAR_PLAYER, playerChar );
  lcd.createChar( CHAR_BULLET, bulletChar );
  lcd.createChar( CHAR_ENEMY, enemyChar );

  // Initialize hardware
  lcd.begin( LCD_COLS, LCD_ROWS );
  Serial.begin( 9600 );
  highscore = EEPROM.read( 200 ) * 255; // Read multiplier
  highscore += EEPROM.read( 201); // Add remainder
  
  welcome( 1 );
  
  for ( int c=0; c<MAX_ENEMIES; c++ ) exist[ c ] = 0;
  for ( int c=0; c<MAX_BULLETS; c++ ) bullet_exists[ c ] = 0;
  
  randomSeed( analogRead( 2 ) );
}

void loop() {
  // Logics
  
  playerLogics.check();
  enemyLogics.check();
  bulletLogics.check();
  checkForInput.check();
  detectCollision.check();
  
  drawScreen.check();
}

// Player logic

void movePlayer() {
  int cur;
  
  if ( ENABLE_HOLD ) cur = currentButton;
  else cur = getSingleButton();
  
  switch ( cur ) {
    case BTN_UP:
    playerPos[ POS_Y ]--;
    break;
    
    case BTN_DOWN:
    playerPos[ POS_Y ]++;
    break;
    
    case BTN_LEFT:
    playerPos[ POS_X ]--;
    break;
    
    case BTN_RIGHT:
    playerPos[ POS_X ]++;
    break;
    
    case BTN_SELECT:
    createBullet();
    break;
  }
  
  if ( playerPos[ POS_Y ] < POS_Y_MIN ) playerPos[ POS_Y ] = POS_Y_MIN;
  else if ( playerPos[ POS_X ] < POS_X_MIN ) playerPos[ POS_X ] = POS_X_MIN;
  else if ( playerPos[ POS_Y ] > POS_Y_MAX ) playerPos[ POS_Y ] = POS_Y_MAX;
  else if ( playerPos[ POS_X ] > POS_X_MAX ) playerPos[ POS_X ] = POS_X_MAX;
  
}

void saveButtonState() {
  currentButton = getButton();
}

int getSingleButton() {
  int cb = getButton();
  if ( lastButton != cb ) {
    lastButton = cb;
    return cb;
  } else return 0;
}

// Bullets logic

void createBullet() {
  for ( int c = 0; c < MAX_BULLETS; c++ ) {
    if ( !bullet_exists[c] ) {
      bullet[c] = new Enemy( playerPos[POS_X] + 1, playerPos[POS_Y] );
      bullet_exists[c] = 1;
      buzzer();
      break;
    }
  }
}

void destroyBullet(int id) {
  delete bullet[id];
  bullet_exists[id] = 0;
}

void moveBullets() {
  for ( int c = 0; c < MAX_BULLETS; c++ ) {
    if ( bullet_exists[c] ) {
      bullet[c]->moveRight();
      if ( bullet[c]->getx() > POS_X_MAX ) {
        destroyBullet( c );
      }
    }
  }
}

// Enemy logic

void drawEnemies() {
  for(int c=0; c<MAX_ENEMIES; c++) {
    if(exist[c]) {
      lcd.setCursor( enemy[c]->getx(), enemy[c]->gety() );
      //lcd.print("*");
      lcd.write( byte( CHAR_ENEMY ) );
    }
  }
}

void moveEnemies() {  
  // Move all enemies
  for(int c=0; c < MAX_ENEMIES; c++) {
    if (exist[c]) {
      enemy[c]->moveLeft();
      if (enemy[c]->getx() < POS_X_MIN) { // If enemy moves out of screen, kill it
        delete enemy[c];
        exist[c] = 0;
        score -= 5;
      }
    }
  }
  
  // Create new enemies
  int rand = random( 1, 100 );
  if( rand < NEW_ENEMY_PROBABILITY ) {
    rand = random( POS_Y_MIN, POS_Y_MAX + 1 );
    for (int c = 0; c < MAX_ENEMIES; c++) {
      if (!exist[c]) {
        enemy[c] = new Enemy(rand);
        exist[c] = 1;
        break;
      }
    }
  }
}

// Game logic

void endgame() {
  lcd.noDisplay();
  buzzdrop();
  delay(300);
  lcd.display();
  delay(300);
  lcd.noDisplay();
  delay(300);
  lcd.display();
  delay(300);
  lcd.noDisplay();
  lcd.clear();
  lcd.display();
  if (score > highscore) {
    int mult;
    int remain;
    mult = score / 255;
    remain = score % 255;
    EEPROM.write(200, mult);
    EEPROM.write(201, remain);
    lcd.print("*  HIGHSCORE!  *");
    buzzdrop(1);
  } else {
    lcd.print("    You lose");
  }
  sc(0,1);
  lcd.print("Score:");
  lcd.print(score);
  delay(1000);
  sc(13,1);
  lcd.print("Rst");
  while(1);
}

void checkCollision() {
  // Collisions between player and enemy
  for(int c=0; c<MAX_ENEMIES; c++) if(exist[c]) if( (enemy[c]->getx() == playerPos[POS_X]) && (enemy[c]->gety() == playerPos[POS_Y])) endgame();
  
  for(int c=0; c<MAX_ENEMIES; c++) if(exist[c]) {
    for (int b=0; b<MAX_BULLETS; b++) if(bullet_exists[b]) {
      if( (bullet[b]->getx() == enemy[c]->getx() ) && (bullet[b]->gety() == enemy[c]->gety() ) ) {
        bulletEnemyCollision( b, c );
      }
      if( (bullet[b]->getx()+1 == enemy[c]->getx() ) && (bullet[b]->gety() == enemy[c]->gety() ) ) {
        bulletEnemyCollision( b, c );
      }
    }
  }
}

void bulletEnemyCollision(int b, int c) {
  destroyBullet(b);
      
  delete enemy[c];
  exist[c] = 0;
  buzzer(800, 50, 2);
  score += 3;
}

void sc() {
  lcd.setCursor(0,1);
}

void sc(int col, int row) {
  lcd.setCursor(col, row);
}

int getButton() {
  int val = getVal();
  if (val > 750) return 0; // No button
  else if (val > 600) return 1; // Select
  else if (val > 350) return 2; // Left
  else if (val > 200) return 3; // Down
  else if (val > 100) return 4; // Up
  else return 5; // Right
}

int getVal() {
  if (analogRead(0)>1000) delay(10);
  return analogRead(0);
}

// Graphics

void updateScreen() {
  lcd.clear();
  drawPlayer();
  drawEnemies();
  drawBullets();
  drawScore();
}

void drawScore() {
  if ( scoreDisplay < 10 ) lcd.setCursor( 15, SCORE_LINE );
  else if ( scoreDisplay < 100 ) lcd.setCursor( 14, SCORE_LINE );
  else if ( scoreDisplay < 1000 ) lcd.setCursor( 13, SCORE_LINE );
  else if ( scoreDisplay < 10000 ) lcd.setCursor( 12, SCORE_LINE );
  else {
    lcd.setCursor( 13, SCORE_LINE );
    lcd.print( ">9K!" );
    return;
  }
  //sc(15,0);
  //lcd.rightToLeft();
  lcd.print( scoreDisplay );
  
  if ( scoreDisplay < score ) scoreDisplay++;
  else if ( scoreDisplay > score ) scoreDisplay--;
  //lcd.leftToRight();
}

void drawBullets() {
  for (int c=0; c<MAX_BULLETS; c++) {
    if (bullet_exists[c]) {
      lcd.setCursor( bullet[c]->getx(), bullet[c]->gety() );
      //lcd.print("-");
      lcd.write( byte( CHAR_BULLET ) );
    }
  }
}

void drawPlayer() {
  lcd.setCursor(playerPos[POS_X], playerPos[POS_Y]);
  //lcd.print(">");
  lcd.write(byte(CHAR_PLAYER));
}

void buzzer() {
  buzzer(DEF_FREQ);
}

void buzzer(int freq) {
  buzzer(freq, 50);
}

void buzzer(int freq, int length) {
  if(buzzEnable) tone(BUZZER_PIN, freq);
  delay(length);
  noTone(BUZZER_PIN);
}

void buzzer(int freq, int length, int rep) {
  for (int c = 0; c < rep; c++) {
    if(buzzEnable) tone(BUZZER_PIN, freq);
    delay(length/2);
    noTone(BUZZER_PIN);
    delay(length/2);
  }
}

void buzzdrop() {
  for (int c = 800; c > 100; c-=50) {
    if(buzzEnable) tone(BUZZER_PIN, c);
    delay(10);
    noTone(BUZZER_PIN);
    delay(10);
  }
  tone(BUZZER_PIN, 100);
  delay(100);
  noTone(BUZZER_PIN);
}

void buzzdrop(int rev) {
  for (int c = 100; c < 800; c+=50) {
    if(buzzEnable) tone(BUZZER_PIN, c);
    delay(10);
    noTone(BUZZER_PIN);
    delay(10);
  }
  tone(BUZZER_PIN, 800);
  delay(100);
  noTone(BUZZER_PIN);
}

void welcome(int val) {
  if ( getButton() == BTN_SELECT ) {
    lcd.clear();
    lcd.print("Clear EEPROM?");
    sc();
    lcd.print("Up to clear");
    while( getButton() == BTN_SELECT || !getButton() );
    if( getButton() == BTN_UP ) {
      EEPROM.write(200, 0);
      EEPROM.write(201, 0);
      lcd.clear();
      lcd.print("EEPROM cleared");
      buzzer(700, 30, 3);
    } else {
      lcd.clear();
      lcd.print("Cancelled");
      buzzer(700, 30, 2);
    }
    delay(1000); 
  } else if ( getButton() == BTN_DOWN ) {
    buzzEnable = !buzzEnable;
    lcd.clear();
    if ( buzzEnable ) lcd.print("Sound on");
    else lcd.print("Sound off");
    delay(1000);
  }
  lcd.clear();
  lcd.print(">           *");
  sc();
  lcd.print("        *");
  delay(100);
  lcd.clear();
  lcd.print(">-         *");
  sc();
  lcd.print("       *");
  delay(100);
  lcd.clear();
  lcd.print("> -       *");
  sc();
  lcd.print("      *");
  delay(100);
  lcd.clear();
  lcd.print("  -      *");
  sc();
  lcd.print(">-   *");
  delay(100);
  lcd.clear();
  lcd.print("   -    *");
  sc();
  lcd.print("> - *");
  delay(100);
  lcd.clear();
  lcd.print("    -  *");
  sc();
  lcd.print("> ");
  delay(60);
  lcd.clear();
  lcd.print("     -*");
  sc();
  lcd.print(" > ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("  > ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("   > ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    > ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    S> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SC> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCR> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCRO> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROL> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLL> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLE> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLER> ");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLER >");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLER  >");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLER   >");
  delay(60);
  lcd.clear();
  sc();
  lcd.print("    SCROLLER    ");
  delay(200);
  sc(12,0);
  lcd.print(VERSION);
  sc(0,0);
  lcd.print("HIGH:");
  sc(5,0);
  lcd.print(highscore);
  while( !getButton() );
}
