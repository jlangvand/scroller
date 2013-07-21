#include <TimedAction.h>

#include <EEPROM.h>

#include <LiquidCrystal.h>

LiquidCrystal lcd(8,9,4,5,6,7);

#define BTN_UP 4
#define BTN_DOWN 3
#define BTN_LEFT 2
#define BTN_RIGHT 5
#define BTN_SELECT 1
#define BTN_NONE 0

#define BUZZER_PIN 12
#define DEF_FREQ 700

#define SCORE_LINE 0

#define POS_X 0
#define POS_Y 1

#define POS_X_MIN 0
#define POS_X_MAX 15
#define POS_Y_MIN 0
#define POS_Y_MAX 1

#define MAX_ENEMIES 7
#define MAX_BULLETS 8

#define DEBUG 1

#define VERSION "v2b2"


/*void Keypad::update() {
  int *key = new int;
  if ( analogRead( *pin ) < 1000 ) delay( 10 );
  int val = analogRead(*pin);
  if ( val > 750 ) *key = 0; // No button
  else if ( val > 600 ) *key = 1; // Select
  else if ( val > 350 ) *key = 2; // Left
  else if ( val > 200 ) *key = 3; // Down
  else if ( val > 100 ) *key = 4; // Up
  else *key = 5; // Right
  
  *last = *current;
  
  *current = *key;
  
  if ( *current > 0 && *last != *current ) *re = *current;
  //else *re = 0;
  
  delete key;
}*/

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

int level=0;

int playerPos[2] = {0,1};

int score = 0;
int highscore;

int buzzEnable = 0;

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

int nextUpdate;

void setup() {
  lcd.begin(16,2);
  Serial.begin(9600);
  highscore = EEPROM.read(206);
  
  welcome(1);
  
  for (int c=0; c<MAX_ENEMIES; c++) exist[c] = 0;
  for (int c=0; c<MAX_BULLETS; c++) bullet_exists[c] = 0;
  
  
  Serial.print("Highscore in EEPROM: ");
  Serial.println(highscore);
  Serial.print("Raw dump addr 0x206: ");
  Serial.println(EEPROM.read(206));
  
  randomSeed(analogRead(2));
  
  nextUpdate = millis() + 100;
}

void loop() {
  // Logics
  
  
  //keypad->update();
  playerLogics.check();
  enemyLogics.check();
  bulletLogics.check();
  checkForInput.check();
  detectCollision.check();
  
  //score++;
  //if (DEBUG) printDebug(enemy.getx(),enemy.gety());
  
  // Graphics
  drawScreen.check();
  
  // Delay
  //delay(100);
}

// Player logic

void movePlayer() {
  switch ( getSingleButton() ) {
    case BTN_UP:
    playerPos[POS_Y]--;
    break;
    
    case BTN_DOWN:
    playerPos[POS_Y]++;
    break;
    
    case BTN_LEFT:
    playerPos[POS_X]--;
    break;
    
    case BTN_RIGHT:
    playerPos[POS_X]++;
    break;
    
    case BTN_SELECT:
    createBullet();
    break;
  }
  
  if (playerPos[POS_Y] < POS_Y_MIN) playerPos[POS_Y] = POS_Y_MIN;
  else if (playerPos[POS_X] < POS_X_MIN) playerPos[POS_X] = POS_X_MIN;
  else if (playerPos[POS_Y] > POS_Y_MAX) playerPos[POS_Y] = POS_Y_MAX;
  else if (playerPos[POS_X] > POS_X_MAX) playerPos[POS_X] = POS_X_MAX;
  
}

void saveButtonState() {
  //if ( getButton() ) currentButton = getButton();
  currentButton = getButton();
}

/*int getSingleButton() {
  if ( lastButton != currentButton ) {
    lastButton = currentButton;
    return currentButton;
  } else return 0;
}*/

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
  //int x = enemy.getx();
  //int y = enemy.gety();
  for(int c=0; c<MAX_ENEMIES; c++) {
    if(exist[c]) {
      lcd.setCursor( enemy[c]->getx(), enemy[c]->gety() );
      lcd.print("*");
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
  int rand = random(1, 100);
  if(rand>80) {
    rand = random(0,2);
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
  delay(500);
  lcd.display();
  delay(500);
  lcd.noDisplay();
  delay(500);
  lcd.display();
  delay(500);
  lcd.noDisplay();
  lcd.clear();
  lcd.display();
  if (score > highscore) {
    EEPROM.write(206, score);
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
  
/*  // Collisions between bullet and enemy
  for(int b=0; b<MAX_BULLETS; b++) {
    if(bullet_exists[b]) {
      for ( int e = 0; e < MAX_ENEMIES; e++ ) {
        if ( exist[e] ) {
          if ( ( enemy[e]->getx() == ( bullet[b]->getx() || bullet[b]->getx() - 1 ) ) && ( enemy[e]->gety() == bullet[b]->gety() ) ) {
            delete enemy[e];
            exist[e] = 0;
            
            destroyBullet(b);
            
            score++;
          }
        }
      }
    }
  }*/
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
  if ( score < 10 ) lcd.setCursor( 15, SCORE_LINE );
  else if ( score < 100 ) lcd.setCursor( 14, SCORE_LINE );
  else if ( score < 1000 ) lcd.setCursor( 13, SCORE_LINE );
  else if ( score < 10000 ) lcd.setCursor( 14, SCORE_LINE );
  else {
    lcd.setCursor( 13, SCORE_LINE );
    lcd.print( ">9K!" );
    return;
  }
  //sc(15,0);
  //lcd.rightToLeft();
  lcd.print(score);
  //lcd.leftToRight();
}

void drawBullets() {
  for (int c=0; c<MAX_BULLETS; c++) {
    if (bullet_exists[c]) {
      lcd.setCursor( bullet[c]->getx(), bullet[c]->gety() );
      lcd.print("-");
    }
  }
}

void drawPlayer() {
  lcd.setCursor(playerPos[POS_X], playerPos[POS_Y]);
  lcd.print(">");
}

/*void printDebug(int ex, int ey) {
  Serial.print("Player pos x: ");
  Serial.print(playerPos[POS_X]);
  Serial.print(" y: ");
  Serial.println(playerPos[POS_Y]);
  Serial.println();
  Serial.print("Enemy pos x: ");
  Serial.print(ex);
  Serial.print(" y: ");
  Serial.println(ey);
  Serial.println();
}*/

void printDebug() {
  Serial.print("Player pos x: ");
  Serial.print(playerPos[POS_X]);
  Serial.print(" y: ");
  Serial.println(playerPos[POS_Y]);
  Serial.println();
  Serial.print("Bullets: ");
  int countBullets = 0;
  for ( int c = 0; c < MAX_BULLETS; c++ ) {
    if ( bullet_exists[c] ) countBullets++;
  }
  Serial.println(countBullets);
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
      EEPROM.write(206, 0);
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
/*  while( !getButton() ) {
    delay(200);
    sc();
    lcd.print("HIGHSCORE: ");
    lcd.print(highscore);
    for(int c=0;c<10;c++) {
      delay(100);
      if( getButton ) break;
    }
    sc();
    lcd.print("    SCROLLER    ");
    for(int c=0;c<10;c++) {
      delay(100);
      if( getButton ) break;
    }
  }*/
}

