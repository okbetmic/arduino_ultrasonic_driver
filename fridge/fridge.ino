#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h> //карта памяти обязательно FAT16 или FAT32

#define ONE_WIRE_BUS 2
#define DELAY 5*1000

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int deviceCount = 0;
float tempC;
unsigned long t = 0;

const int chipSelect = 10;

void setup(void){
  Serial.begin(9600);
  
   if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  
  //SD.begin(chipSelect);
  
  sensors.begin();
  if(Serial){
    Serial.print("Locating devices...");
    Serial.print("Found ");
  }
  deviceCount = sensors.getDeviceCount();
  if(Serial){
    Serial.print(deviceCount, DEC);
    Serial.println(" devices.");
    Serial.println("");

    Serial.println("millis\texternal\tinternal_top\tinternal_bottom");
  }
  File dataLog = SD.open("startlog.txt", FILE_WRITE);//проверка
  dataLog.println("Hello World!");
  dataLog.close();
  
}

void loop(void){
  if ((unsigned long)(millis() - t) >= DELAY){
    File dataLog = SD.open("startlog.txt", FILE_WRITE);
    dataLog.print(t);
    
    t = millis();
    if(Serial){
      Serial.print(t);
      Serial.print("\t");
    }
    
    sensors.requestTemperatures();
    for (int i = 0;  i < deviceCount;  i++){
      tempC = sensors.getTempCByIndex(i);
      if(Serial){
        Serial.print(tempC);
        Serial.print("\t");
      }
      dataLog.print('\t');
      dataLog.print(tempC);
    }
    dataLog.print('\n');
    dataLog.close();
    if(Serial)
      Serial.println("");

  }
}
