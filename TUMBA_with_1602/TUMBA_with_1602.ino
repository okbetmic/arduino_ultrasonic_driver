#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AD9850.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "GyverEncoder.h"
#include "GyverButton.h"

//#define DEBUG

//EEPROM
#define FREQ_ADDR 0
#define STEP_ADDR 4

//LCD pins
#define I2C_ADR 0x27
#define symbolscount 16
#define stringscount 2
#define desk_size 2
int desktop = 0;

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

#define MAX_STEP 5
#define MIN_STEP 0
long step[MAX_STEP + 1] = {1, 5, 10, 100, 1000, 10000}; //массив шагов

#define STEP_CHANGE_CNT 2
#define F_DELAY 0

#define PHASE 0
#define MIN_FREQ 0
#define MAX_FREQ 1000000

#define NEW_TIME 1000
#define TEMP_TIME 1000
#define REWRITE_SCREEN_TIME 10000000

//jump
#define BUTTON_PIN 12
#define DELTA_JUMP 0.001
#define SCAN_DELAY 1e6 //microseconds
#define JUMP_COUNT 500
#define AD_DELAY 786.4 //microseconds


//temperature cordinates
int temp_cords[3][2] = {
  {2, 0},//top
  {2, 1},//bot
  {10, 0}//water
};
//temperature sensors addresses
uint8_t temp_addr[3][8] = {
  {0x28, 0xFF, 0x61, 0x5D, 0x67, 0x14, 0x02, 0x65},//top
  {0x28, 0xFF, 0xFA, 0x15, 0x15, 0x15, 0x02, 0x3A},//bot
  {0x28, 0xFF, 0x6C, 0xF9, 0x53, 0x14, 0x01, 0x8C} //water
};

 GButton jump_button(BUTTON_PIN);
Encoder E(EN_CLK, EN_DT, EN_SW);
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
  Serial.begin(9600);
  freq = EEPROMReadlong(FREQ_ADDR);
  freq_step = EEPROMReadlong(STEP_ADDR);
  DDS.begin(AD_W_CLK, AD_FQ_UD, AD_DATA_D7, RESET);

  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);

  if (Serial) {
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
    Serial.println("");

    Serial.println("millis\texternal\tinternal_top\tinternal_bottom");
  }

  E.setType(TYPE1);

  lcd.begin();
  lcd.clear();

  screen_write();

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
  jump_button.tick();
  DDS.setfreq(freq, PHASE);

  if (jump_button.isSingle()) //нажата кнопка прыжка
    freq_jump();

  if(E.isClick()){ //Если двойное нажатие, то меняем экран 
    desktop++;
    desktop%= desk_size;
    screen_write();
  }

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

void freq_change(){
  if(desktop == 0)
    write_freq();

  EEPROMWritelong(FREQ_ADDR, freq);
}

void step_change(){
  if(desktop == 0)
    write_step();

  EEPROMWritelong(STEP_ADDR, freq_step);
}

void screen_write(){
  screen_clear();

  if(desktop == 0)
    write_frequency_screen();
  if(desktop == 1)
    write_temperature_screen();
}


void write_frequency_screen(){
  write_freq();
  write_step();
}

void write_freq(){
  lcd.setCursor(6, 0);
  lcd.print("         ");
  lcd.setCursor(0, 0);
  lcd.print("FREQ = ");
  lcd.print(freq);
}

void write_step(){
  lcd.setCursor(6, 1);
  lcd.print("         ");
  lcd.setCursor(0, 1);
  lcd.print("STEP = ");
  lcd.print(step[freq_step]);
}

void write_temperature_screen(){
  screen_clear();

  lcd.setCursor(0, 0);
  lcd.print("U:");

  lcd.setCursor(0, 1);
  lcd.print("D:");

  lcd.setCursor(8, 0);
  lcd.print("W:");

  lcd.setCursor(10, 1);
  lcd.print(":  :");

  write_new_temperature();
  write_time();
}


void write_new_temperature(){
  if(desktop != 1)
    return;
    
  for(int i = 0; i < 3; i++){
    lcd.setCursor(temp_cords[i][0], temp_cords[i][1]);
    tempC = sensors.getTempC(temp_addr[i]);
  
    if(tempC == 85.00 || tempC == -127.00 || (tempC < 10 && tempC > 9)) {
      lcd.print("     ");
      lcd.setCursor(temp_cords[i][0], temp_cords[i][1]);
    }

    if(tempC == 85.00 || tempC == -127.00)
      lcd.print("d:[");
    else
      lcd.print(tempC);

    if(Serial){
      Serial.print(millis());
      Serial.print(tempC);
      Serial.print("\t");
    }
  }
}

void write_time() {
  if(desktop != 1)
    return;
  
  long tim[3];
  tim[0] = last_time / 1000 / 60 / 60;
  tim[1] = (last_time / 1000 / 60) % 60;
  tim[2] = (last_time / 1000) % 60;

  for(int i = 0; i < 3; i++) {
    lcd.setCursor(i * 3 + 8, 1);
    if(tim[i] / 10 == 0) {
      lcd.print(0);
      lcd.setCursor(i * 3 + 1 + 8, 1);
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

void screen_clear(){
  for(int i = 0; i < stringscount; i++){
    lcd.setCursor(0, i);
    for(int j = 0; j < symbolscount; j++)
      lcd.print(" ");
  }
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
