//#define CHECK

#include <TFT.h>
#include <SPI.h>

volatile unsigned int int_tic = 0;
volatile unsigned long tic;

int dataPin  = 4;   //Пин подключен к DS входу 74HC595
int latchPin = 6;  //Пин подключен к ST_CP входу 74HC595
int clockPin = 7; //Пин подключен к SH_CP входу 74HC595
volatile int mode_out = 4;//начальный режим работы

long int old_f;

#ifdef CHECK
long int f_now;
#endif

volatile int pre_mode_out = mode_out;
volatile unsigned long long pre_millis;

#define background_color  0, 0, 0
#define freq_color  255, 255, 255
#define mode_count_color  0, 0, 255
#define change_mode_count_color  0, 255, 0
#define limit_color 255, 0, 0
#define string_color 240, 230, 140

#define FREQ_X_SPACE 10
#define FREQ_Y_SPACE 10
#define FREQ_SIGN_SPACE 3 //расстояние между знаками в выводе частоты
#define HZ_SPACE 10 // расстояние между значением частоты и "Hz"
#define BOUNCE_DELAY_TIME 50 //время задержки на считывании кнопок (от дребезжания)
#define LIMIT_SPACE 10 //расстояние между строкой режимов и лимитами
#define STRING_SPACE 10

#define LIMIT_SIZE 1 //размер лимитов
#define MODE_SIZE 3 //размер символов режима
#define F_SIZE 2 //размер символов частоты
#define STRING_SIZE 30

#define MODE_L 7  //расстояние между номерами
#define MODE_SPACE 10 //расстояние от нижней точки значеня частоты до прямой, на которой лежит строка режимов

#define MIN_MODE 1 //наименьший номер режима
#define MAX_MODE 7 //наибольший номер режима

#define OUT_T 50 //период через который на экране будут обновляться значения частоты

#define CS   10
#define DC   9
#define RESET  8
#define buttonOne 2
#define buttonTwo 3

long int limit[7][2] = {
  {792, 4596},
  {1120, 6497},
  {2806, 16249},
  {6308, 36189},
  {19031, 106000},
  {58184, 298250},
  {110166, 340000}
};


TFT tft = TFT(CS, DC, RESET);

void setup() {
  delay(1000); //время наподумать
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
  //out = String(mode_out);

  if (mode_out == 4)
    num = 1 << (4);

  if (mode_out == 5)
    num = 1 << 3;

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
  if (old_f != tic)
    freq_out(tic);

#ifdef CHECK
  int pmo = pre_mode_out;
  for(long int i = limit[mode_out - 1][0]; i < limit[mode_out - 1][1]; i+= 0.01*i){
      if(pmo!= pre_mode_out){
        break;
        stringClear();
      }  
      f_now = i;
      freq_out(i);
      delay(OUT_T);
  }    

  for(long int i = limit[mode_out - 1][1]; i > limit[mode_out - 1][0]; i-= 0.01*i){
      if(pmo!= pre_mode_out){
        break;
        stringClear();
      } 
      f_now = i;
      freq_out(i);
      delay(OUT_T);
  } 
#endif
    
  delay(OUT_T);
}

void initialization() {
  tft.background(background_color);
  tft.setTextSize(F_SIZE);

  String freq = get_f(tic);

  for (int i = 0; i < freq.length(); i++)
    freq_writing(freq_color, freq[i], i);
  
  hz_writing(freq_color, freq.length());;

  old_f = tic;


  tft.setTextSize(MODE_SIZE);
  tft.stroke(mode_count_color);
  for (int i = 1; i <= MAX_MODE; i++) {
    String out = String(i);
    tft.text(out.c_str(), (i - 1) * (5 * MODE_SIZE) + MODE_L * i, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE);
  }

  String out = String(mode_out);
  tft.stroke(change_mode_count_color);
  tft.text(out.c_str(), (mode_out - 1) * (5 * MODE_SIZE) + MODE_L * mode_out, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE);

  limitsOut();
  stringClear();
  stringOut();
}


String get_f(long int f){
  if(String(f).length() > 3)
    return  String(f).substring(0, String(f).length() - 3) + " " + String(f).substring(String(f).length() - 3, String(f).length());
  else
    return String(f); 
}

void freq_out(long int f) {
  String freq = get_f(f);
  String pre_freq = get_f(old_f);

  if (freq.length() == pre_freq.length())
    for (int i = 0; i < freq.length(); i++) 
      if (freq[i] != pre_freq[i]) {
        freq_writing(background_color, pre_freq[i], i);
        freq_writing(freq_color, freq[i], i);
      }

  if (freq.length() > pre_freq.length()) {
    hz_writing(background_color, pre_freq.length());

    for (int i = 0; i < pre_freq.length(); i++) {
      if (freq[i] != pre_freq[i]) {
        freq_writing(background_color, pre_freq[i], i);
        freq_writing(freq_color, freq[i], i);
      }
    }
    for (int i = pre_freq.length(); i < freq.length(); i++)
        freq_writing(freq_color, freq[i], i);

    hz_writing(freq_color, freq.length());    
  }

  if (freq.length() < pre_freq.length()) {
    hz_writing(background_color, pre_freq.length());

    for (int i = 0; i < freq.length(); i++) 
      if (freq[i] != pre_freq[i]) {
        freq_writing(background_color, pre_freq[i], i);
        freq_writing(freq_color, freq[i], i);
      }
    for (int i = freq.length(); i < pre_freq.length(); i++)
      freq_writing(background_color, pre_freq[i], i);

    hz_writing(freq_color, freq.length());
  }

  stringOut();
  
  old_f = f;
}

void freq_writing(int r, int g, int b, char value, int pos){
  tft.setTextSize(F_SIZE);
  tft.stroke(r, g, b);
  char a[2] = {value, 0};
  tft.text(a, (F_SIZE * 5 + FREQ_SIGN_SPACE) * pos + FREQ_X_SPACE, FREQ_Y_SPACE);
}

void hz_writing(int r, int g, int b, int pos){
  tft.setTextSize(F_SIZE);
  char hz[3] = {'H', 'z', 0};
  tft.stroke(r, g, b);
  tft.text(hz, (F_SIZE * 5 + FREQ_SIGN_SPACE) * pos + HZ_SPACE   + FREQ_X_SPACE, FREQ_Y_SPACE);
  }


void mode_down() {
  if (digitalRead(buttonTwo) == LOW)
    initialization();

  if (millis() - pre_millis > BOUNCE_DELAY_TIME) {
    if (mode_out > MIN_MODE){
      mode_out--;
      modeOut();
    }

    pre_mode_out = mode_out;
    pre_millis = millis();
  }
}

void mode_up() {
  if (digitalRead(buttonOne) == LOW)
    initialization();

  if (millis() - pre_millis > BOUNCE_DELAY_TIME) {
    if (mode_out < MAX_MODE){
      mode_out++;
      modeOut();
    }
    pre_mode_out = mode_out;
    pre_millis = millis();
  }
}

void modeOut() {
  limitsOut();
  
  tft.setTextSize(MODE_SIZE);
  tft.stroke(background_color);
  
  String out = String(pre_mode_out);
  tft.stroke(mode_count_color);
  tft.text(out.c_str(), (pre_mode_out - 1) * (5 * MODE_SIZE) + MODE_L * pre_mode_out, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE);

  int num = 1 << (mode_out - 1);
  out = String(mode_out);

  if (mode_out == 4)
    num = 1 << (4);

  if (mode_out == 5)
    num = 1 << 3;

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, num);
  digitalWrite(latchPin, HIGH);

  tft.stroke(change_mode_count_color);
  tft.text(out.c_str(), (mode_out - 1) * (5 * MODE_SIZE) + MODE_L * mode_out, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE);

  stringClear();
}


void limitsOut(){
  tft.setTextSize(LIMIT_SIZE);
  
  tft.stroke(background_color);
  tft.text(String(limit[pre_mode_out - 1][0]).c_str(), 0, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE);
  tft.stroke(limit_color);
  tft.text(String(limit[mode_out - 1][0]).c_str(), 0, F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE);
  
  tft.stroke(background_color);
  tft.text(String(limit[pre_mode_out - 1][1]).c_str(), tft.width() - String(limit[pre_mode_out - 1][1]).length() * LIMIT_SIZE * (5+1), F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE);
  tft.stroke(limit_color);
  tft.text(String(limit[mode_out - 1][1]).c_str(), tft.width() - String(limit[mode_out - 1][1]).length() * LIMIT_SIZE * (5+1), F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE);
}

void stringOut(){
  long int f = tic;
  long int pre_f = old_f;
  int y = F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE + STRING_SPACE;
#ifdef CHECK
  f = f_now;
#endif

  if(f > limit[mode_out - 1][1]) f = limit[mode_out - 1][1];
  if(f < limit[mode_out - 1][0]) f = limit[mode_out - 1][0];
  if(pre_f > limit[mode_out - 1][1]) pre_f = limit[mode_out - 1][1];
  if(pre_f < limit[mode_out - 1][0]) pre_f = limit[mode_out - 1][0];

  long int map_f = map(f, limit[mode_out - 1][0], limit[mode_out - 1][1], 0, tft.width());
  long int map_pre_f = map(pre_f, limit[mode_out - 1][0], limit[mode_out - 1][1], 0, tft.width());

  if(map_f > map_pre_f){
    tft.fill(string_color);
    tft.noStroke();
    tft.rect(0, y, map_f, STRING_SIZE);
  }
  if(map_f < map_pre_f){
    tft.fill(background_color);
    tft.noStroke();
    tft.rect(map_f, y, map_pre_f - map_f, STRING_SIZE);
  }
}

void stringClear(){
  int y = F_SIZE * 8 + MODE_SPACE + FREQ_Y_SPACE + MODE_SIZE * 8 + LIMIT_SPACE + STRING_SPACE;
  long int map_f = map(tic, limit[mode_out - 1][0], limit[mode_out - 1][1], 0, tft.width());

#ifdef CHECK
  map_f = map(f_now, limit[mode_out - 1][0], limit[mode_out - 1][1], 0, tft.width());
#endif
  
  tft.fill(background_color);
  tft.noStroke();
  tft.rect(0, y, tft.width(), STRING_SIZE);

  tft.fill(string_color);
  tft.noStroke();
  tft.rect(0, y, map_f, STRING_SIZE);
}
