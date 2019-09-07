#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>

#define ONE_WIRE_BUS 2
#define DELAY 30*1000

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int deviceCount = 0;
float tempC;
unsigned long t = 0;

const int chipSelect = 10;

void setup(void)
{
  SD.begin(chipSelect);
  
  sensors.begin();
  Serial.begin(9600);
  Serial.print("Locating devices...");
  Serial.print("Found ");
  deviceCount = sensors.getDeviceCount();
  Serial.print(deviceCount, DEC);
  Serial.println(" devices.");
  Serial.println("");

  Serial.println("millis\texternal\tinternal_top\tinternal_bottom");

}

void loop(void){
  if ((unsigned long)(millis() - t) >= DELAY){
    File dataFile = SD.open("fridgelog.txt", FILE_WRITE);
    
    t = millis();
    Serial.print(t);
    Serial.print("\t");

    sensors.requestTemperatures();
    for (int i = 0;  i < deviceCount;  i++){
      tempC = sensors.getTempCByIndex(i);
      Serial.print(tempC);
      Serial.print("\t");
      dataFile.print(tempC);
      dataFile.print('\t');
    }
    dataFile.print('\n');
    Serial.println("");

  }
}
