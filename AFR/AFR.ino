#include <AccelStepper.h>

#define STEP 13
#define DIR 12

#define external_driver 1 // - режим "external driver" (A4988)

AccelStepper StepMotor(external_driver, STEP, DIR); 
int step_size = 1;

volatile unsigned int int_tic = 0;
volatile unsigned long tic;
#define battery 5      // пин измерения напряжения
int volts, watts;    // храним вольты и ватты
float ohms;          // храним омы
float my_vcc_const;  // константа вольтметра

long int old_f;

void setup() {
  StepMotor.setMaxSpeed(3000); //устанавливаем максимальную скорость вращения ротора двигателя (шагов/секунду)
  StepMotor.setAcceleration(13000); //устанавливаем ускорение (шагов/секунду^2)

  delay(1000); //время наподумать
  Serial.begin(9600);
  pinMode (5, INPUT); // вход сигнала T1 (only для atmega328)
  
  TCCR2A = 1 << WGM21; //CTC mode
  TIMSK2 = 1 << OCIE2A; OCR2A = 124 ; //прерывание каждые 8мс
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20); //делитель 1024

  TCCR1A = 0; TIMSK1 = 1 << TOIE1; //прерывание по переполнению
  TCCR1B = (1 << CS10) | (1 << CS11) | (1 << CS12); //тактировани от входа Т1

  long int f_out = tic;

  int n = 1000; //количество оборотов двигателя

  for(int i = 0; i < n; i++){
    motor_run();
    if(f_out != tic){
      Serial.print(tic);
      Serial.print("/t");
      Serial.println(readVcc());
      }
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

void motor_run(){
  StepMotor.run();
  StepMotor.move(step_size);
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
