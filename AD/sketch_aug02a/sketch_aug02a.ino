#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "GyverEncoder.h"
#include <AD9850.h>

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

#define MAX_STEP 100000
#define MIN_STEP 1

#define F_DELAY 0

#define PHASE 0
#define MIN_FREQ 1
#define MAX_FREQ 1000000

Encoder E(EN_CLK, EN_DT, EN_SW);
LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount);

long freq_step = 1;
long freq = 1;


void setup() {
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
  if (E.isRight() && freq + freq_step <= MAX_FREQ) {
    freq += freq_step;
    freq_change();
    delay(F_DELAY);
  }

  if (E.isLeft() && freq - freq_step >= MIN_FREQ) {
    freq -= freq_step;
    freq_change();
    delay(F_DELAY);
  }

  if (E.isRightH()) {
    if (freq_step < MAX_STEP)
    {
      freq_step *= 10;
      step_change();
      delay(F_DELAY);
    }
  }
  if (E.isLeftH()) {
    if (freq_step > MIN_STEP)
    {
      freq_step /= 10;
      step_change();
      delay(F_DELAY);
    }
  }
}

void freq_change() {
  DDS.setfreq(freq, PHASE);

  lcd.setCursor(6, 0);
  lcd.print("          ");
  lcd.setCursor(6, 0);
  lcd.print(freq);
}

void step_change()
{
  lcd.setCursor(6, 1);
  lcd.print("          ");
  lcd.setCursor(6, 1);
  lcd.print(freq_step);
}