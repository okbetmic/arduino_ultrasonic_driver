#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "GyverEncoder.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x);
#else
#define DEBUG_PRINT(x)
#endif

//EEPROM
#define FREQ_ADDR 0
#define STEP_ADDR 4

//LCD pins
#define I2C_ADR 0x38
#define symbolscount 20
#define stringscount 4
#define desk_size 2

//DDS AD pins
#define AD_W_CLK 8
#define AD_FQ_UD 7
#define AD_DATA_D7 6
#define RESET 5

//Encoder pins
#define EN_CLK 2
#define EN_DT 3
#define EN_SW 4

#define ONE_WIRE_BUS 10 //DS18B20 pins


long step[] = {1, 5, 10, 100, 1000, 10000}; //freq steps array
#define MIN_STEP 0
#define MAX_STEP sizeof(step)/sizeof(step[MIN_STEP])

#define STEP_CHANGE_CNT 2
#define F_DELAY 0

#define PHASE 0
#define MIN_FREQ 0
#define MAX_FREQ 1000000

#define NEW_TIME 1000
#define TEMP_TIME 1000
#define REWRITE_SCREEN_TIME 10000000

//temperature cordinates
int temp_cords[3][2] = {
  {4, 2},//top
  {4, 3},//bot
  {14, 2}//water
};
//temperature sensors addresses
uint8_t temp_addr[3][8] = {
  {0x28, 0xFF, 0x61, 0x5D, 0x67, 0x14, 0x02, 0x65},//top
  {0x28, 0xFF, 0xCA, 0x69, 0x15, 0x15, 0x02, 0x4},//bot
  {0x28, 0xFF, 0x6C, 0xF9, 0x53, 0x14, 0x01, 0x8C} //water
};

Encoder E(EN_CLK, EN_DT, EN_SW, TYPE2);
LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

long freq_step = 1;
long freq = 0;
long step_change_counter = 0;
bool temp_step = 0;
float tempC;
unsigned long last_temp = 0, last_time = 0, last_rewrite = 0;

void setup() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

  freq = EEPROMReadlong(FREQ_ADDR);
  freq_step = EEPROMReadlong(STEP_ADDR);
  DDS.begin(AD_W_CLK, AD_FQ_UD, AD_DATA_D7, RESET);

  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);

  DEBUG_PRINT(sensors.getDeviceCount());
  DEBUG_PRINT(" devices.");
  DEBUG_PRINT("");
  DEBUG_PRINT("millis\tinternal_top\tinternal_bottom\twater");

  lcd.begin();
  lcd.clear();

  write_freq();
  write_step();
  write_temperature_screen();

  DDS.setfreq(freq, PHASE);
}

void loop() {

  if (millis() - last_time > NEW_TIME) {
    last_time = millis() - millis() % 1000;
    write_time();
  }

  if (millis() - last_temp > TEMP_TIME) {
    last_temp = millis();
    if (!temp_step)
      sensors.requestTemperatures();
    else
      write_new_temperature();

    temp_step = !temp_step;
  }


  E.tick();
  DDS.setfreq(freq, PHASE);

  if (E.isRight() && freq + step[freq_step] <= MAX_FREQ) {//изменение частоты в большую сторону
    freq += step[freq_step];
    freq_change();
    delay(F_DELAY);
  }

  if (E.isLeft() && freq - step[freq_step] >= MIN_FREQ) {//изменение частоты в меньшую сторону
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
  write_freq();

  EEPROMWritelong(FREQ_ADDR, freq);
}

void step_change() {
  write_step();

  EEPROMWritelong(STEP_ADDR, freq_step);
}

void screen_write() {
  screen_clear();

  write_frequency_screen();
  write_temperature_screen();
}


void write_frequency_screen() {
  write_freq();
  write_step();
}

void write_freq() {
  lcd.setCursor(6, 0);
  lcd.print("         ");
  lcd.setCursor(0, 0);
  lcd.print("FREQ = ");
  lcd.print(freq);
}

void write_step() {
  lcd.setCursor(6, 1);
  lcd.print("         ");
  lcd.setCursor(0, 1);
  lcd.print("STEP = ");
  lcd.print(step[freq_step]);
}

void write_temperature_screen() {
  screen_clear();

  lcd.setCursor(temp_cords[0][0] - 4, temp_cords[0][1]);
  lcd.print("U = ");

  lcd.setCursor(temp_cords[1][0] - 4, temp_cords[1][1]);
  lcd.print("D = ");

  lcd.setCursor(temp_cords[2][0] - 4, temp_cords[2][1]);
  lcd.print("W = ");

  lcd.setCursor(symbolscount - 6, stringscount - 1);
  lcd.print(":  :");

  write_new_temperature();
  write_time();
}


void write_new_temperature() {

  DEBUG_PRINT("\n")
  DEBUG_PRINT(millis())
  DEBUG_PRINT("\t")

  for (int i = 0; i < 3; i++) {
    lcd.setCursor(temp_cords[i][0], temp_cords[i][1]);
    tempC = sensors.getTempC(temp_addr[i]);
    DEBUG_PRINT(tempC)
    DEBUG_PRINT("\t")

    if (tempC == 85.00 || tempC == -127.00 || (tempC < 10 && tempC > 9) ) {
      lcd.print("     ");
      lcd.setCursor(temp_cords[i][0], temp_cords[i][1]);
    }

    if (tempC == 85.00 || tempC == -127.00)
      lcd.print("d:[");
    else
      lcd.print(tempC);
  }
}

void write_time() {
  long tim[3];
  tim[0] = last_time / 1000 / 60 / 60;
  tim[1] = (last_time / 1000 / 60) % 60;
  tim[2] = (last_time / 1000) % 60;

  for (int i = 0; i < 3; i++) {
    lcd.setCursor(i * 3 + (symbolscount - 8), stringscount - 1);
    if (tim[i] / 10 == 0) {
      lcd.print(0);
      lcd.setCursor(i * 3 + 1 + (symbolscount - 8), stringscount - 1);
    }
    lcd.print(tim[i]);
  }
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

void screen_clear() {
  for (int i = 0; i < stringscount; i++) {
    lcd.setCursor(0, i);
    for (int j = 0; j < symbolscount; j++)
      lcd.print(" ");
  }
}
