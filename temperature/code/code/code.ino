#include <OneWire.h>

byte addr[3][8] = {
  {28, FF, 61, 5D, 67, 14, 2, 65},
  {28, FF, FA, 15, 15, 15, 2, 3A},
  {28, FF, 94, 13, 15, 15, 2, 80}
};

void setup(){
  Serial.begin(9600);
  }

void loop(){
  
  }
