#include <TFT.h>
#include <SPI.h>

volatile unsigned int int_tic = 0;
volatile unsigned long tic;

int dataPin  = 4;   //Пин подключен к DS входу 74HC595
int latchPin = 6;  //Пин подключен к ST_CP входу 74HC595
int clockPin = 7; //Пин подключен к SH_CP входу 74HC595
volatile int mode_out = 4;//начальный режим работы

long int old_f;

volatile int pre_mode_out = mode_out;
volatile unsigned long long pre_millis;

#define freq_color  0, 255, 255
#define mode_count_color  0, 0, 255
#define change_mode_count_color  0, 255, 0
#define background_color  0, 0, 0

#define FREQ_SIGN_SPACE 3 //расстояние между знаками в выводе частоты
#define HZ_SPACE 10 // расстояние между значением частоты и "Hz"
#define BOUNCE_DELAY_TIME 30 //время задержки на считывании кнопок (от дребезжания)
#define LIMITS_HEIGHT 20
#define LIMIT_SIZE 1

#define MODE_SIZE 3 //размер символов режима
#define F_SIZE 3 //размер текста частоты

#define MODE_L 7  //расстояние между номерами
#define MODE_HEIGHT 10 //расстояние от нижней точки значеня частоты до прямой, на которой лежит строка режимов

#define MIN_MODE 1 //наименьший номер режима
#define MAX_MODE 7 //наибольший номер режима

#define OUT_T 50 //период через который на экране будут обновляться значения частоты

#define CS   10
#define DC   9
#define RESET  8
#define buttonOne 2
#define buttonTwo 3

int limit[7][2] = {
  {690, 3700},
  {1150, 6200},
  {2800, 15000},
  {6700, 36000},
  {20000, 100000},
  {62000, 293000},
  {119000, 340000}
};


TFT tft = TFT(CS, DC, RESET);

void setup() {
  //Serial.begin(9600);
  pinMode (5, INPUT); // вход сигнала T1 (only для atmega328)

  TCCR2A = 1 << WGM21; //CTC mode
  TIMSK2 = 1 << OCIE2A; OCR2A = 124 ; //прерывание каждые 8мс
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); //делитель 1024

  TCCR1A = 0; TIMSK1 = 1 << TOIE1; //прерывание по переполнению
  TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12); //тактировани от входа Т1

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(buttonOne, INPUT_PULLUP);
  pinMode(buttonTwo, INPUT_PULLUP);

  int num = 1 << (mode_out - 1);

  digitalWrite(latchPin, LOW);                        // устанавливаем синхронизацию "защелки" на LOW
  shiftOut(dataPin, clockPin, LSBFIRST, num);   // передаем последовательно на dataPin
  digitalWrite(latchPin, HIGH);                       //"защелкиваем" регистр, тем самым устанавливая значения на выходах

  attachInterrupt(buttonOne - 2, mode_down, LOW);
  attachInterrupt(buttonTwo - 2, mode_up, LOW);

  tft.begin();
  tft.background(background_color);

  initialization();
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

void loop() {
  //Serial.println(tic);
  /*if (old_f != tic)
    freq_out(tic);*/
  for(long int i = 0; i < 1000000; i+=99 + 0.01 * i){  
    freq_out(i);
    delay(OUT_T);
  }

}

String get_f(long int f){
  if(String(f).length() > 3)
    return  String(f).substring(0, String(f).length() - 3) + " " + String(f).substring(String(f).length() - 3, String(f).length());
  else
    return String(f); 
}

void freq_out(long int f) {
  tft.setTextSize(F_SIZE);

  char hz[3] = {'H', 'z', 0};
  String freq = get_f(f);
  String pre_freq = get_f(old_f);

  if (freq.length() == pre_freq.length()) {
    for (int i = 0; i < freq.length(); i++) {
      if (freq[i] != pre_freq[i]) {
        tft.stroke(background_color);
        char a[2] = {pre_freq[i], 0};
        tft.text(a, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);

        tft.stroke(freq_color);
        char b[2] = {freq[i], 0};
        tft.text(b, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
      }
    }
  }

  if (freq.length() > pre_freq.length()) {
    tft.stroke(background_color);
    tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * (pre_freq.length()) + HZ_SPACE, 0);

    for (int i = 0; i < pre_freq.length(); i++) {
      if (freq[i] != pre_freq[i]) {
        tft.stroke(background_color);
        char a[2] = {pre_freq[i], 0};
        tft.text(a, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);

        tft.stroke(freq_color);
        char b[2] = {freq[i], 0};
        tft.text(b, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
      }
    }
    for (int i = pre_freq.length(); i < freq.length(); i++) {
      tft.stroke(freq_color);
      char b[2] = {freq[i], 0};
      tft.text(b, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
    }

    tft.stroke(freq_color);
    tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * (freq.length()) + HZ_SPACE, 0);
  }

  if (freq.length() < pre_freq.length()) {
    tft.stroke(background_color);
    tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * (pre_freq.length()) + HZ_SPACE, 0);

    for (int i = 0; i < freq.length(); i++) {
      if (freq[i] != pre_freq[i]) {
        tft.stroke(background_color);
        char a[2] = {pre_freq[i], 0};
        tft.text(a, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);

        tft.stroke(freq_color);
        char b[2] = {freq[i], 0};
        tft.text(b, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
      }
    }
    for (int i = freq.length(); i < pre_freq.length(); i++) {
      tft.stroke(background_color);
      char a[2] = {pre_freq[i], 0};
      tft.text(a, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
    }

    tft.stroke(freq_color);
    tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * freq.length() + HZ_SPACE, 0);
  }

  old_f = f;
}

void mode_down() {
  if (digitalRead(buttonTwo) == LOW)
    initialization();

  if (millis() - pre_millis > BOUNCE_DELAY_TIME) {
    if (mode_out > MIN_MODE)
      mode_out--;
    modeOut();

    pre_mode_out = mode_out;
    pre_millis = millis();
  }
}

void mode_up() {
  if (digitalRead(buttonOne) == LOW)
    initialization();

  if (millis() - pre_millis > BOUNCE_DELAY_TIME) {
    if (mode_out < MAX_MODE)
      mode_out++;
    modeOut();
    pre_mode_out = mode_out;
    pre_millis = millis();
  }
}

void modeOut() {
  tft.setTextSize(MODE_SIZE);
  tft.stroke(background_color);
  
  String out = String(pre_mode_out);
  tft.stroke(mode_count_color);
  tft.text(out.c_str(), (pre_mode_out - 1) * (5 * MODE_SIZE) + MODE_L * pre_mode_out, F_SIZE * 8 + MODE_HEIGHT);
  tft.stroke(change_mode_count_color);

  int num = 1 << (mode_out - 1);
  out = String(mode_out);

  if (mode_out == 4)
    num = 1 << (4);

  if (mode_out == 5)
    num = 1 << 3;

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, num);
  digitalWrite(latchPin, HIGH);

  tft.text(out.c_str(), (mode_out - 1) * (5 * MODE_SIZE) + MODE_L * mode_out, F_SIZE * 8 + MODE_HEIGHT);
}


void initialization() {
  tft.background(background_color);
  tft.setTextSize(F_SIZE);

  char hz[3] = {'H', 'z', 0};
  String freq = get_f(tic);

  for (int i = 0; i < freq.length(); i++) {
    tft.stroke(freq_color);
    char b[2] = {freq[i], 0};
    tft.text(b, (F_SIZE * 5 + FREQ_SIGN_SPACE) * i, 0);
  }
  
  tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * freq.length() + HZ_SPACE, 0);


  tft.setTextSize(MODE_SIZE);
  tft.stroke(mode_count_color);
  for (int i = 1; i <= 7; i++) {
    String out = String(i);
    tft.text(out.c_str(), (i - 1) * (5 * MODE_SIZE) + MODE_L * i, F_SIZE * 8 + MODE_HEIGHT);
  }

  String out = String(mode_out);
  tft.stroke(change_mode_count_color);
  tft.text(out.c_str(), (mode_out - 1) * (5 * MODE_SIZE) + MODE_L * mode_out, F_SIZE * 8 + MODE_HEIGHT);
}
