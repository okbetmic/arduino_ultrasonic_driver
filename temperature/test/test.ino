#include <OneWire.h>

OneWire ds(2); // на пине 10 (нужен резистор 4.7 КОм)

void setup(void) {

Serial.begin(9600);

}

void loop(void) {

byte i;

byte present = 0;

byte type_s;

byte data[12];

byte addr[8];

float t;

if ( !ds.search(addr)) {

Serial.println("No more addresses.");

Serial.println();

ds.reset_search();

delay(250);

return;

}

Serial.print("ROM =");

for( i = 0; i < 8; i++) {

Serial.write(' ');

Serial.print(addr[i], HEX);

}

if (OneWire::crc8(addr, 7) != addr[7]) {

Serial.println("CRC is not valid!");

return;

}

Serial.println();

// первый байт определяет чип

switch (addr[0]) {

case 0x10:

Serial.println(" Chip = DS18S20"); // или более старый DS1820

type_s = 1;

break;

case 0x28:

Serial.println(" Chip = DS18B20");

type_s = 0;

break;

case 0x22:

Serial.println(" Chip = DS1822");

type_s = 0;

break;

default:

Serial.println("Device is not a DS18x20 family device.");

return;

}

ds.reset();

ds.select(addr);

ds.write(0x44); // начинаем преобразование, используя ds.write(0x44,1) с "паразитным" питанием

delay(1000); // 750 может быть достаточно, а может быть и не хватит

// мы могли бы использовать тут ds.depower(), но reset позаботится об этом

present = ds.reset();

ds.select(addr);

ds.write(0xBE);

Serial.print(" Data = ");

Serial.print(present, HEX);

Serial.print(" ");

for ( i = 0; i < 9; i++) { // нам необходимо 9 байт

data[i] = ds.read();

Serial.print(data[i], HEX);

Serial.print(" ");

}

Serial.print(" CRC=");

Serial.print(OneWire::crc8(data, 8), HEX);

Serial.println();

// конвертируем данный в фактическую температуру

// так как результат является 16 битным целым, его надо хранить в

// переменной с типом данных "int16_t", которая всегда равна 16 битам,

// даже если мы проводим компиляцию на 32-х битном процессоре

int16_t raw = (data[1] << 8) | data[0];

if (type_s) {

raw = raw << 3; // разрешение 9 бит по умолчанию

if (data[7] == 0x10) {

raw = (raw & 0xFFF0) + 12 - data[6];

}

} else {

byte cfg = (data[4] & 0x60);

// при маленьких значениях, малые биты не определены, давайте их обнулим

if (cfg == 0x00) raw = raw & ~7; // разрешение 9 бит, 93.75 мс

else if (cfg == 0x20) raw = raw & ~3; // разрешение 10 бит, 187.5 мс

else if (cfg == 0x40) raw = raw & ~1; // разрешение 11 бит, 375 мс
}

t = (float)raw / 16.0;

Serial.print(" Temp = ");
Serial.print(t);
}
