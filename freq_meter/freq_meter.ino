#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

volatile unsigned int int_tic = 0;
volatile unsigned long tic;

long int old_f;

#define I2C_ADR 0x27
#define symbolscount 20
#define stringscount 4

int dataPin  = 4;   //Пин подключен к DS входу 74HC595
int latchPin = 6;  //Пин подключен к ST_CP входу 74HC595
int clockPin = 7; //Пин подключен к SH_CP входу 74HC595

#define OUT_T 50 //период через который на экране будут обновляться значения частоты


LiquidCrystal_I2C lcd(I2C_ADR, symbolscount, stringscount); //i2c-адрес, кол-во символов, кол-во строк

void setup(){
   delay(1000); //время наподумать
  //Serial.begin(9600);
  pinMode (5, INPUT); // вход сигнала T1 (only для atmega328)
  
  TCCR2A = 1 << WGM21; //CTC mode
  TIMSK2 = 1 << OCIE2A; OCR2A = 124 ; //прерывание каждые 8мс
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); //делитель 1024

  TCCR1A = 0; TIMSK1 = 1 << TOIE1; //прерывание по переполнению
  TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12); //тактировани от входа Т1

  
  lcd.begin();
  lcd.backlight();

  long int f_out = tic;  
}


ISR (TIMER1_OVF_vect) {
  int_tic++;
}

ISR (TIMER2_COMPA_vect) {
  static byte n = 1;
  if (n == 125) {
    tic = ((uint32_t)int_tic << 16) | TCNT1; //сложить что натикало
    int_tic = 0;
    TCNT1 = 0; n = 0;
  }
  n++;
}


void loop(){
  if (old_f != tic)
    freq_out(tic);
}


void freq_out(long int f) {
  lcd.clear();
  lcd.print(f);
  
  old_f = f;
}
