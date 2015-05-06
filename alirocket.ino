#include <SFE_BMP180.h>
#include <Wire.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

SFE_BMP180 pressure;

double baseline; // baseline pressure

float curMaxAlti; 

int incomingByte = 0;   // for incoming serial data

struct config_t
{
    float min;
    float max;
} configuration;


void setup()
{
  mySerial.begin(9600);
  Serial.begin(9600);
  mySerial.println("REBOOT");

  if (pressure.begin())
    mySerial.println("BMP180 init success");
  else
  {
    mySerial.println("BMP180 init fail (disconnected?)\n\n");
    while(1); // Pause forever.
  }

  baseline = getPressure();
  
  mySerial.print("baseline pressure: ");
  mySerial.print(baseline);
  mySerial.println(" mb");
  
  EEPROM_readAnything(0, configuration);
  mySerial.print("EEPROM Max: ");
  mySerial.println(configuration.max);
}

void loop()
{
  double a,P;
  
  // Get a new pressure reading:

  P = getPressure();

  // Show the relative altitude difference between
  // the new reading and the baseline reading:

  a = pressure.altitude(P,baseline);
  
  mySerial.print("relative altitude: ");
  if (a >= 0.0) mySerial.print(" "); // add a space for positive numbers
  mySerial.print(a,1);
  mySerial.print(" meters, ");
  if (a >= 0.0) mySerial.print(" "); // add a space for positive numbers
  mySerial.print(a*3.28084,0);
  mySerial.println(" feet");
  mySerial.print("EEPROM Max: ");
  mySerial.print(configuration.max);
  mySerial.print(" Min: ");
  mySerial.println(configuration.min);

  Serial.print("relative altitude: ");
  if (a >= 0.0) Serial.print(" "); // add a space for positive numbers
  Serial.print(a,1);
  Serial.print(" meters, ");
  if (a >= 0.0) Serial.print(" "); // add a space for positive numbers
  Serial.print(a*3.28084,0);
  Serial.println(" feet");
  Serial.print("EEPROM Max: ");
  Serial.print(configuration.max);
  Serial.print(" Min: ");
  Serial.println(configuration.min);
  
  boolean change = false;
  
  if (a > (configuration.max + 1)){
     configuration.max = a;
     change = true;
  }
  //if (a < configuration.min){
     //configuration.min = a;
     //change = true; 
  //}

  
  if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      if (incomingByte == 97){
        Serial.println("Got Command");
        configuration.max = 0;
        configuration.min = 0;
        change = true;
      }
      
  }
  
  if (change == true){
     mySerial.println("EEPROM");
     EEPROM_writeAnything(0, configuration);
  }
  
  delay(500);
}


double getPressure()
{
  char status;
  double T,P,p0,a;

  // You must first get a temperature measurement to perform a pressure reading.
  
  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:

    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Use '&T' to provide the address of T to the function.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P.
        // Use '&P' to provide the address of P.
        // Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          return(P);
        }
        else mySerial.println("error retrieving pressure measurement\n");
      }
      else mySerial.println("error starting pressure measurement\n");
    }
    else mySerial.println("error retrieving temperature measurement\n");
  }
  else mySerial.println("error starting temperature measurement\n");
}



