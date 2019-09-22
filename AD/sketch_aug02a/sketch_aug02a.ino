#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850.h>
#include <EEPROM.h>
#include "GyverEncoder.h"
#include "GyverButton.h"

#define DEBUG

//EEPROM
#define FREQ_ADR 0

//LCD pins
#define I2C_ADR 0x27
#define symbolscount 20
#define stringscount 4

//DDS AD pins
#define AD_W_CLK 6
#define AD_FQ_UD 7
#define AD_DATA_D7 8
#define RESET 9

//Encoder pins
#define EN_CLK 2
#define EN_DT 3
#define EN_SW 4

#define MAX_STEP 11
#define MIN_STEP 0
int step[MAX_STEP+1] = {1, 5, 10, 50, 100, 300, 500, 1000, 2000, 3000, 5000, 100000}; //массив шагов

#define STEP_CHANGE_CNT 2
#define F_DELAY 0

#define PHASE 0
#define MIN_FREQ 0
#define MAX_FREQ 1000000

//jump
#define BUTTON_PIN 12
#define DELTA_JUMP 0.001
#define SCAN_DELAY 200*1e3 //microseconds
#define JUMP_COUNT 10


GButton jump_button(BUTTON_PIN);
Encoder E(EN_CLK, EN_DT, EN_SW);
LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount);

long freq_step = 1;
long freq = 0;
int step_change_counter = 0;

void setup() {
  freq = EEPROMReadlong(FREQ_ADR);
  
  DDS.begin(AD_W_CLK, AD_FQ_UD, AD_DATA_D7, RESET);

  E.setType(TYPE1);

  lcd.begin();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("freq: ");
  lcd.setCursor(6, 0);
  lcd.print(freq);

  lcd.setCursor(0, 1);
  lcd.print("step: ");
  lcd.setCursor(6, 1);
  lcd.print(freq_step);

  DDS.setfreq(freq, PHASE);
}

void loop() {

  E.tick();
  jump_button.tick();
  DDS.setfreq(freq, PHASE);

  if(jump_button.isSingle()){ //нажата кнопка прыжка
#ifdef DEBUG
    Serial.println("start jumping");
#endif
    freq_jump();
  }

  if (E.isRight() && freq + freq_step <= MAX_FREQ) {//изменение частоты в большую сторону
    freq += step[freq_step];
    freq_change();
    delay(F_DELAY);
  }

  if (E.isLeft() && freq - freq_step >= MIN_FREQ) {//изменение частоты в меньшую сторону
    freq -= step[freq_step];
    freq_change();
    delay(F_DELAY);
  }

  if (E.isRightH()) {//изменение шага в большую сторону
    if (step_change_counter == STEP_CHANGE_CNT)
    {
      step_change_counter = 0;
      if (freq_step < MAX_STEP)
      {
        freq_step++;
        step_change();
        delay(F_DELAY);
      }
    }
    else
      step_change_counter++;

  }
  if (E.isLeftH()) {//изменение шага в меньшую сторону
    if (step_change_counter == STEP_CHANGE_CNT)
    {
      step_change_counter = 0;
      if (freq_step > MIN_STEP)
      {
        freq_step--;
        step_change();
        delay(F_DELAY);
      }
    }
    else
      step_change_counter++;
  }
  if (E.isRelease())
    step_change_counter = 0;
}

void freq_change() {
  lcd.setCursor(6, 0);
  lcd.print("          ");
  lcd.setCursor(6, 0);
  lcd.print(freq);

  EEPROMWritelong(FREQ_ADR, freq);
}

void step_change()
{
  lcd.setCursor(6, 1);
  lcd.print("          ");
  lcd.setCursor(6, 1);
  lcd.print(step[freq_step]);
}

long long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
 
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
 
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

void freq_jump(){
  int n = JUMP_COUNT;
  while(n--){
#ifdef DEBUG
    Serial.println(n);
#endif
    for(long double i = 0; i <= step[freq_step]; i+= (long double)step[freq_step] * DELTA_JUMP){
      DDS.setfreq(freq + i, PHASE);
      delayMicroseconds(SCAN_DELAY * DELTA_JUMP);
    }
  }
}
