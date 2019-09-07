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
  File dataFridge = SD.open("fridgelog.txt", FILE_WRITE);
  File dataLog = SD.open("startlog.txt", FILE_WRITE);//проверка
  dataLog.println("Hello World!");
  dataLog.close();
  dataFridge.println("start logging");
  dataFridge.close();
}

void loop(void){
  if ((unsigned long)(millis() - t) >= DELAY){
    File dataFridge = SD.open("fridgelog.txt", FILE_WRITE); //Возможно нужно создать файл на сд карте перед тем как её туда вставлять
    dataFridge.print(t);
    
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
      dataFridge.print('\t');
      dataFridge.print(tempC);
    }
    dataFridge.print('\n');
    dataFridge.close();
    if(Serial)
      Serial.println("");

  }
}
