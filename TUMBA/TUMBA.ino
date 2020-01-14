#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
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
#define AD_W_CLK 8
#define AD_FQ_UD 7
#define AD_DATA_D7 6
#define RESET 5

//Encoder pins
#define EN_CLK 2
#define EN_DT 3
#define EN_SW 4

#define ONE_WIRE_BUS 10 //температура

#define MAX_STEP 11
#define MIN_STEP 0
long step[MAX_STEP + 1] = {1, 5, 10, 50, 100, 300, 500, 1000, 2000, 3000, 5000, 10000}; //массив шагов

#define STEP_CHANGE_CNT 2
#define F_DELAY 0

#define PHASE 0
#define MIN_FREQ 0
#define MAX_FREQ 1000000

//jump
#define BUTTON_PIN 12
#define DELTA_JUMP 0.001
#define SCAN_DELAY 20*1e3 //microseconds
#define JUMP_COUNT 100
#define AD_DELAY 786.4 //microseconds


GButton jump_button(BUTTON_PIN);
Encoder E(EN_CLK, EN_DT, EN_SW);
LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

long freq_step = 1;
long freq = 0;
long step_change_counter = 0;
int deviceCount = 0;
float tempC;
unsigned long t = 0, last_time = 0;

void setup() {
  Serial.begin(9600);
  freq = EEPROMReadlong(FREQ_ADR);
  DDS.begin(AD_W_CLK, AD_FQ_UD, AD_DATA_D7, RESET);

  sensors.begin();
  deviceCount = sensors.getDeviceCount();

  if(Serial){
    Serial.print(deviceCount, DEC);
    Serial.println(" devices.");
    Serial.println("");

    Serial.println("millis\texternal\tinternal_top\tinternal_bottom");
  }

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

  lcd.setCursor(0, 2);
  lcd.print("u = ");
  lcd.setCursor(7, 2);
  lcd.print("d = ");
  lcd.setCursor(14, 2);
  lcd.print("w = ");

  for(int i = 0; i < 3; i++){
    lcd.setCursor(i * 3 + 2, 3);
    lcd.print(":");
  }

  DDS.setfreq(freq, PHASE);
}

void loop() {
  if(Serial){
      Serial.print(t);
      Serial.print("\t");
  }

  if(last_time - millis() > 1000){
    last_time = millis();

    long tim[3];
    tim[0] = last_time / 1000 / 60 / 60;
    tim[1] = (last_time / 1000 / 60) % 60;
    tim[2] = (last_time / 1000) % 60;

    for(int i = 0; i < 3; i++){
      lcd.setCursor(i * 3, 3);
      if(tim[i] / 10 == 0){
        lcd.print(0);
        lcd.setCursor(i*3 + 1, 3);  
      }
      lcd.print(tim[i]);
    }
  }

  sensors.requestTemperatures();
  for (int i = 0;  i < deviceCount;  i++){
    tempC = sensors.getTempCByIndex(i);
    lcd.setCursor(7*i + 4, 2);
    lcd.print(tempC);
    if(Serial){
      Serial.print(tempC);
      Serial.print("\t");
    }
  }

  for (int i = 0;  i < deviceCount;  i++){
    tempC = sensors.getTempCByIndex(i);
  }
  
  E.tick();
  jump_button.tick();
  DDS.setfreq(freq, PHASE);

  if (jump_button.isSingle()) { //нажата кнопка прыжка
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

void freq_jump() {
  int n = JUMP_COUNT;
  long double count = SCAN_DELAY / (AD_DELAY * 2);
  while (n--) {
#ifdef DEBUG
    Serial.println(n);
#endif
    digitalWrite(13, 1);
    digitalWrite(13, 0);
    for (long double i = 0; i <= step[freq_step]; i += (long double)step[freq_step] / count) {
      DDS.setfreq(freq + i, PHASE);
      delayMicroseconds(AD_DELAY);
    }
  }
}
