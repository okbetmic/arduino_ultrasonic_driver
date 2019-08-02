#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "GyverEncoder.h"
#include <AD9850.h> 

//LCD pins
#define I2C_ADR 0x27
#define symbolscount 20
#define stringscount 4

//DDS AD pins
#define AD_W_CLK 
#define AD_FQ_UD 
#define AD_DATA_D7 
#define RESET

//Encoder pins
#define EN_CLK 7
#define EN_DT 8
#define EN_SW 9


#define phase 0
#define freq_min 1
#define freq_max 9999999

Encoder E(EN_CLK, EN_DT, EN_SW);
LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount); //i2c-адрес, кол-во символов, кол-во строк

long long int freq_step = 1000000;
int freq = 1;


void setup() {
  DDS.begin(AD_W_CLK, AD_FQ_UD, AD_DATA_D7, RESET);
  lcd.begin();
  lcd.clear();
  lcd.print(freq);
  DDS.setfreq(freq, phase);
}

void loop() {
  if(E.isRight() && freq + freq_step <= freq_max){
    freq+= freq_step;
    freq_change();
  }
  
  if(E.isRight() && freq - freq_step >= freq_min){
    freq-= freq_step;
    freq_change();
  }
  
  if(E.isClick()){
    step_change();  
  }
}

void freq_change(){
  DDS.setfreq(freq, phase);

  lcd.clear();
  lcd.print(freq);
}

void step_change(){
  if(freq_step == 1)
    freq_step = 1000000;
  else
    freq_step/= 10;
}
