#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
   
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <SoftwareSerial.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

SoftwareSerial mySerial(12, 11); // RX, TX

float curAlti, baseline, temperature,clock; 
int incomingByte = 0;   // for incoming serial DOWNLINK
boolean updateStats = false;
boolean first = true;
int ESET,ESTAT;

int countdown=20;
int fps=1000;
int missionStatus; // 0 - standby mode 1-countdown mode 2-launch mode

int red = 7;
int blue = 8;
int green = 9;

struct config_t
{
    float min;
    float max;
    float alti;
    float temp;
    int clock;
} stats;

struct _settings
{
    boolean DEBUG;
    boolean DOWNLINK;
    boolean DATA;
    
} settings;

void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  
  if (settings.DEBUG){
    Serial.println("------------------------------------");
    Serial.println("                 DEBUG              ");
    Serial.println("------------------------------------");
    Serial.print  ("FW:            "); Serial.println("v0.1");
    Serial.print  ("Sensor:        "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:    "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:     "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:     "); Serial.print(sensor.max_value); Serial.println(" hPa");
    Serial.print  ("Min Value:     "); Serial.print(sensor.min_value); Serial.println(" hPa");
    Serial.print  ("Resolution:    "); Serial.print(sensor.resolution); Serial.println(" hPa");  
    Serial.print  ("Stats Lenght:  "); Serial.println(ESTAT); 
    Serial.print  ("Settings Start: "); Serial.println(ESET); 
    Serial.println("------------------------------------");
    Serial.println("");
  }
  
  mySerial.println("------------------------------------");
  mySerial.print  ("Sensor:       "); mySerial.println(sensor.name);
  mySerial.print  ("Driver Ver:   "); mySerial.println(sensor.version);
  mySerial.print  ("Unique ID:    "); mySerial.println(sensor.sensor_id);
  mySerial.print  ("Max Value:    "); mySerial.print(sensor.max_value); mySerial.println(" hPa");
  mySerial.print  ("Min Value:    "); mySerial.print(sensor.min_value); mySerial.println(" hPa");
  mySerial.print  ("Resolution:   "); mySerial.print(sensor.resolution); mySerial.println(" hPa");  
  mySerial.print  ("Stats Lenght:   "); mySerial.println(ESTAT); 
  mySerial.println("------------------------------------");
  mySerial.println("");
  
  delay(500);
}

void setup()
{ 
  mySerial.begin(9600);
  Serial.begin(9600);
  
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  
  mySerial.println("SYSTEM REBOOT");
  Serial.println("SYSTEM REBOOT");

  ESTAT = EEPROM_readAnything(0, stats);
  
  Serial.println(stats.max);
  ESET = ESTAT+1;
  EEPROM_readAnything(ESET, settings);
  
  /* Initialise the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Sensor Error - System Halt");
    mySerial.print("Sensor Error - System Halt");

    while(1);
  }
  clock = millis();
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  
  
}

void loop()
{
  
  if (missionStatus == 1){
    
    switch(countdown){
      default:
       Serial.print("T-");
       Serial.println(countdown);
       mySerial.print("T-");
       mySerial.println(countdown);
     case 20:
       settings.DOWNLINK = true;
       settings.DEBUG = true;
       settings.DATA = true;
       break;
     case 0:
       fps = 10;
       missionStatus = 2;
       Serial.println("*********************Launch***********************");
       mySerial.println("*********************Launch***********************");
       break;
     case 2:
       clock = millis();
       break;
    }

    countdown--;
  
  }
  
    /* Get a new sensor event */ 
  sensors_event_t event;
  bmp.getEvent(&event);
 
  /* Display the results (barometric pressure is measure in hPa) */
  if (event.pressure && settings.DATA == true)
  {
      /* Display atmospheric pressue in hPa */
      
      if (settings.DEBUG){
        Serial.print("Pressure:    ");
        Serial.print(event.pressure);
        Serial.println(" hPa");
      }
      if (settings.DOWNLINK){
      mySerial.print("Pressure:    ");
      mySerial.print(event.pressure);
      mySerial.println(" hPa");
      }
      
       
      /* First we get the current temperature from the BMP085 */
      bmp.getTemperature(&temperature);
      
      if (settings.DEBUG){
          Serial.print("Temperature: ");
          Serial.print(temperature);
          Serial.println(" C");
      }
      if (settings.DOWNLINK){
          mySerial.print("Temperature: ");
          mySerial.print(temperature);
          mySerial.println(" C");
      }
  
      /* Then convert the atmospheric pressure, and SLP to altitude         */
      /* Update this next line with the current SLP for better results      */
      float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
      
      curAlti = bmp.pressureToAltitude(seaLevelPressure,event.pressure);
      
      if (first){
          first = false;
          baseline = curAlti;
      }
      
      if (settings.DEBUG){
          Serial.print("Altitude:    "); 
          Serial.print(curAlti); 
          Serial.println(" m");
          Serial.print("Ali Change:    "); 
          Serial.print(curAlti-baseline); 
          Serial.println(" m");
          Serial.println("");
        
      }
      if (settings.DOWNLINK){
          mySerial.print("Altitude:    "); 
          mySerial.print(curAlti); 
          mySerial.println(" m");
          mySerial.print("Ali Change:    "); 
          mySerial.print(curAlti-baseline); 
          mySerial.println(" m");
          mySerial.println("");
      }
  }
  else
  {
    Serial.println("Sensor error");
  }
  
  if (settings.DOWNLINK){
      mySerial.print("Max Alti: ");
      mySerial.print(stats.max);
      mySerial.print(" Temp @ Max: ");
      mySerial.print(stats.temp);
      mySerial.print(" (R)Altitude(m): ");
      mySerial.print(stats.alti);
      mySerial.print(" Clock: ");
      mySerial.print(stats.clock);
      mySerial.println("");
  }


  if (settings.DEBUG){
      Serial.print("Max Alti: ");
      Serial.print(stats.max);
      Serial.print(" Temp @ Max: ");
      Serial.print(stats.temp);
      Serial.print(" (R)Altitude(m): ");
      Serial.print(stats.alti);
      Serial.print(" Clock: ");
      Serial.print(stats.clock);
      Serial.println("");

  }
    
  if (curAlti > stats.max && missionStatus == 2){
       stats.max = curAlti;
       stats.temp = temperature;
       stats.clock = (millis() - clock)-2000;
       stats.alti = curAlti - baseline;
       saveStats();
  }
  //if (curAlti < stats.min){
       //stats.min = a;
       //change = true; 
  //}

 if (Serial.available() > 0 || mySerial.available() > 0) {
      // read the incoming byte:
      
      if (Serial.available() > 0) {
          incomingByte = Serial.read();
      }else{
          incomingByte = mySerial.read();
      }
      Serial.println(incomingByte);
      switch (incomingByte) {
          case 'd':
          case 'D':
            if (settings.DEBUG == true){
              settings.DEBUG = false;
            }
            else if (settings.DEBUG == false){
              settings.DEBUG = true;
            }
            saveSettings();
            break;
          case 'c':
          case 'C':
            if (settings.DEBUG)Serial.println("Clear Stats");
            if (settings.DOWNLINK)mySerial.println("Clear Stats");
    
            stats.max = 0;
            stats.min = 0;
            stats.temp = 0;
            saveStats();
            break;
          case 'x':
          case 'X':
            if (settings.DOWNLINK == true){
                settings.DOWNLINK = false;
            }
            else if (settings.DOWNLINK == false){
                settings.DOWNLINK = true;
            }
            saveSettings();
            break;
          case 's':
          case 'S':
            printStatus();
            break;
          case '?':
          case 'h':
            printHelp();
            break;
          case 'r':
          case 'R':
            if (settings.DATA == true){
                settings.DATA = false;
            }
            else if (settings.DATA == false){
                settings.DATA = true;
            }
            saveSettings();
          case 13://n
          case 10://r
            break;
          case 42:
            missionStatus = 1;
            fps = 850;
            Serial.println("Starting CountDown");
            break;
          case 33: //!
            missionStatus = 0;
            Serial.println("DATA LOCK");
            mySerial.println("DATA LOCK");
            fps = 1000;
            break;
          case 'u':
          case 'U':
            if (settings.DEBUG){
              Serial.print("Altitude:    "); 
              Serial.print(curAlti); 
              Serial.println(" m");
              Serial.print("Ali Change:    "); 
              Serial.print(curAlti-baseline); 
              Serial.println(" m");
              Serial.println("");
              
            }
            if (settings.DOWNLINK){
              mySerial.print("Altitude:    "); 
              mySerial.print(curAlti); 
              mySerial.println(" m");
              mySerial.print("Ali Change:    "); 
              mySerial.print(curAlti-baseline); 
              mySerial.println(" m");
              mySerial.println("");
            }
            break;
          default:
            //Invalid option
            Serial.print("\n");
            Serial.println("error: invalid option");
            Serial.println("Enter h or ? for help");
            
            mySerial.print("\n");
            mySerial.println("error: invalid option");
            mySerial.println("Enter h or ? for help");
            break;
      }
        
  }
  if (missionStatus == 0){
    digitalWrite(red,HIGH);
    digitalWrite(blue,LOW);
    digitalWrite(green,LOW);
  }
  if (missionStatus == 1){
    digitalWrite(red,LOW);
    digitalWrite(blue,HIGH);
    digitalWrite(green,LOW);
  }
  if (missionStatus == 2){
    digitalWrite(red,LOW);
    digitalWrite(blue,LOW);
    digitalWrite(green,HIGH);
  }
  
  delay(fps);  
  
  if (missionStatus == 0){
    digitalWrite(red,HIGH);
    digitalWrite(blue,LOW);
    digitalWrite(green,LOW);
  }else{
    digitalWrite(blue,LOW);
    digitalWrite(green,LOW);
    digitalWrite(red,LOW);
  }
    
    


}

void printStatus(){
    Serial.print("\n");
    mySerial.print("\n");

    Serial.print("Debug Status: ");
    Serial.println(settings.DEBUG);
    mySerial.print("Debug Status: ");
    mySerial.println(settings.DEBUG);
    
    Serial.print("2.4GHz Status: ");
    Serial.println(settings.DOWNLINK);
    mySerial.print("2.4GHz Status: ");
    mySerial.println(settings.DOWNLINK);
     
    Serial.print("Data Status: ");
    Serial.println(settings.DATA);
    mySerial.print("Data Status: ");
    mySerial.println(settings.DATA);
    
    mySerial.print("\n");
    Serial.print("\n");
    
      
}

void saveSettings(){
  
    EEPROM_writeAnything(ESET, settings);
    
    Serial.println("Settings Saved");
    mySerial.println("Settings Saved");
}

void saveStats(){
  
    EEPROM_writeAnything(0, stats);
    
    Serial.println("New Stat Saved");
    mySerial.println("New Stat Saved");
}

void printHelp(){
      Serial.print("\n");
      Serial.println("Commands:");
      Serial.print("d/D - Debug Output On/Off - ");
      Serial.println(settings.DEBUG);
      Serial.print("x/X - 2.4 GHz Downlink On/Off - ");
      Serial.println(settings.DOWNLINK);
      Serial.println("c/C - Clear Stats");
      Serial.println("s/S - Print Current Settings");
      Serial.println("h/? - This Help Message");  
      Serial.print("\n");
      
      mySerial.print("\n");
      mySerial.println("Commands:");
      mySerial.print("d/D - Debug Output On/Off - ");
      mySerial.println(settings.DEBUG);
      mySerial.print("x/X - 2.4 GHz Downlink On/Off - ");
      mySerial.println(settings.DOWNLINK);
      mySerial.println("c/C - Clear Stats");
      mySerial.println("s/S - Print Current Settings");
      mySerial.println("h/? - This Help Message");
      mySerial.print("\n");  
}

