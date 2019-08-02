#define Y_STP     3
#define Y_DIR     6
#define EN        8
#define external_driver 1 // - режим "external driver" (A4988)

#include <EEPROMex.h>   // библиотека для работы со внутренней памятью ардуино
#define initial_calibration 1

int step_size = 1;

volatile unsigned int int_tic = 0;
volatile unsigned long tic;
#define battery 5      // пин измерения напряжения
int volts, watts;    // храним вольты и ватты
float ohms;          // храним омы
float my_vcc_const;  // константа вольтметра

long int old_f;

long int f_out = tic;

int delayTime = 30; //Delay between each pause (uS)
int stps = 6400; // Steps to move

void step(boolean dir, byte dirPin, byte stepperPin, int steps)
{
  digitalWrite(dirPin, dir);
  delay(100);
  for (int i = 0; i < steps; i++) {
    digitalWrite(stepperPin, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(stepperPin, LOW);
    delayMicroseconds(delayTime);
  }
}

void calibration() {
  //--------калибровка----------
  for (byte i = 0; i < 7; i++) EEPROM.writeInt(i, 0);          // чистим EEPROM для своих нужд
  my_vcc_const = 1.1;
  Serial.print("Real VCC is: "); Serial.println(readVcc());     // общаемся с пользователем
  Serial.println("Write your VCC (in millivolts)");
  while (Serial.available() == 0); int Vcc = Serial.parseInt(); // напряжение от пользователя
  float real_const = (float)1.1 * Vcc / readVcc();              // расчёт константы
  Serial.print("New voltage constant: "); Serial.println(real_const, 3);
  EEPROM.writeFloat(8, real_const);                             // запись в EEPROM
  while (1); // уйти в бесконечный цикл
  //------конец калибровки-------
}

void setup() {
  delay(1000); //время наподумать
  Serial.begin(9600);
  pinMode (5, INPUT); // вход сигнала T1 (only для atmega328)
  pinMode(Y_DIR, OUTPUT); pinMode(Y_STP, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(EN, LOW);
  
  if (initial_calibration) calibration();  // калибровка, если разрешена

  my_vcc_const = EEPROM.readFloat(8);
  
  TCCR2A = 1 << WGM21; //CTC mode
  TIMSK2 = 1 << OCIE2A; OCR2A = 124 ; //прерывание каждые 8мс
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); //делитель 1024

  TCCR1A = 0; TIMSK1 = 1 << TOIE1; //прерывание по переполнению
  TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12); //тактировани от входа Т1

  delay(2000);
  int n = 0; //количество оборотов двигателя
  for (int i = 0; i < n; i++) {
    motor_run();
    if (abs(f_out - tic) >= 5) {
      Serial.print(tic);
      Serial.print("/t");
      Serial.println(readVcc());
      f_out = tic;
    }
    delay(200);
  }
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
}

void motor_run() {
  step(false, Y_DIR, Y_STP, stps);
}

long readVcc() { //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин)
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result; // расчёт реального VCC
  return result; // возвращает VCC
}
