#include <Wire.h> 
#include <Adafruit_Sensor.h> 
#include <Adafruit_BMP085_U.h>
#include <EEPROM.h> 
#include "EEPROMAnything.h"
#include <SoftwareSerial.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

SoftwareSerial mySerial(12, 11); // RX, TX

float curAlti, baseline, temperature, clock;
int incomingByte = 0; // for incoming serial DOWNLINK
boolean updateStats = false;
boolean first = true;
int ESET, ESTAT;

int countdown = 20;
int fps = 1000;
int missionStatus; // 0 - standby mode 1-countdown mode 2-launch mode

int red = 7;
int blue = 8;
int green = 9;

float time;

struct config_t {
    float min;
    float max;
    float alti;
    float temp;
    int maxClock;
    int missionTime;
}
stats;

struct _settings {
    boolean DEBUG;
    boolean DOWNLINK;
    boolean DATA;

}
settings;

void displaySensorDetails(void) {
    sensor_t sensor;
    bmp.getSensor( & sensor);


    sendSerialDataln("------------------------------------");
    sendSerialDataln("                 DEBUG              ");
    sendSerialDataln("------------------------------------");
    sendSerialData("FW:            ");
    sendSerialDataln("v0.1");
    sendSerialData("Sensor:        ");
    sendSerialDataln(String(sensor.name));
    sendSerialData("Driver Ver:    ");
    sendSerialDataln(String(sensor.version));
    sendSerialData("Unique ID:     ");
    sendSerialDataln(String(sensor.sensor_id));
    sendSerialData("Max Value:     ");
    sendSerialData(String(sensor.max_value));
    sendSerialDataln(" hPa");
    sendSerialData("Resolution:    ");
    sendSerialData(String(sensor.resolution));
    sendSerialDataln(" hPa");
    sendSerialData("Stats Lenght:  ");
    sendSerialDataln(String(ESTAT));
    sendSerialDataln("------------------------------------");
    sendSerialDataln("");



    delay(500);
}

void setup() {
    mySerial.begin(9600);
    Serial.begin(9600);

    pinMode(red, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(blue, OUTPUT);

    sendSerialDataln("SYSTEM REBOOT");

    ESTAT = EEPROM_readAnything(0, stats);
    if (isnan(stats.max)) {
        stats.max = 0;
        stats.min = 0;
        stats.alti = 0;
        stats.temp = 0;
        stats.maxClock = millis();
    }
    ESET = ESTAT + 1;
    EEPROM_readAnything(ESET, settings);

    /* Initialise the sensor */
    if (!bmp.begin()) {
        /* There was a problem detecting the BMP085 ... check your connections */
        sendSerialData("Sensor Error - System Halt");
        while (1);
    }
    clock = millis();
    /* Display some basic information on this sensor */
    displaySensorDetails();

}

void loop() {
    time = millis();

    if (missionStatus == 1) {

        switch (countdown) {
            default: sendSerialData("T-");
            sendSerialDataln(String(countdown));
        case 20:
                settings.DOWNLINK = true;
            settings.DEBUG = true;
            settings.DATA = true;
            break;
        case 0:
                fps = 10;
            missionStatus = 2;
            sendSerialDataln("*********************Launch***********************");
            break;
        case 2:
                clock = millis();
            break;
        }

        countdown--;

    }

    /* Get a new sensor event */
    sensors_event_t event;
    bmp.getEvent( & event);

    /* Display the results (barometric pressure is measure in hPa) */
    if (event.pressure && settings.DATA == true) {
        /* Display atmospheric pressue in hPa */


        sendSerialData("Pressure:    ");
        sendSerialData(String(event.pressure));
        sendSerialDataln(" hPa");


        /* First we get the current temperature from the BMP085 */
        bmp.getTemperature( & temperature);


        sendSerialData("Temperature: ");
        sendSerialData(String(temperature));
        sendSerialDataln(" C");

        /* Then convert the atmospheric pressure, and SLP to altitude         */
        /* Update this next line with the current SLP for better results      */
        float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

        curAlti = bmp.pressureToAltitude(seaLevelPressure, event.pressure);

        if (first) {
            first = false;
            baseline = curAlti;
        }

        sendSerialData("Altitude:    ");
        sendSerialData(String(curAlti));
        sendSerialDataln(" m");
        sendSerialData("Ali Change:    ");
        sendSerialData(String(curAlti - baseline));
        sendSerialDataln(" m");
        sendSerialDataln("");


    } else {
        sendSerialDataln("Sensor error");
    }

    sendSerialData("Max Alti: ");
    sendSerialData(String(stats.max));
    sendSerialData(" Temp @ Max: ");
    sendSerialData(String(stats.temp));
    sendSerialData(" (R)Altitude(m): ");
    sendSerialData(String(stats.alti));
    sendSerialData("Max Alti Time ");
    sendSerialData(String(stats.maxClock));
    sendSerialDataln("");

    if (curAlti > stats.max && missionStatus == 2) {
        stats.max = curAlti;
        stats.temp = temperature;
        stats.maxClock = (millis() - clock) - 2000;
        stats.alti = curAlti - baseline;
        saveStats();
    }

    if (Serial.available() > 0 || mySerial.available() > 0) {
        // read the incoming byte:

        if (Serial.available() > 0) {
            incomingByte = Serial.read();
        } else {
            incomingByte = mySerial.read();
        }
        switch (incomingByte) {
        case 'd':
        case 'D':
            if (settings.DEBUG == true) {
                settings.DEBUG = false;
            } else if (settings.DEBUG == false) {
                settings.DEBUG = true;
            }
            saveSettings();
            break;
        case 'c':
        case 'C':
            sendSerialDataln("Clear Stats");

            stats.max = 0;
            stats.min = 0;
            stats.temp = 0;
            saveStats();
            break;
        case 'x':
        case 'X':
            if (settings.DOWNLINK == true) {
                settings.DOWNLINK = false;
            } else if (settings.DOWNLINK == false) {
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
            if (settings.DATA == true) {
                settings.DATA = false;
            } else if (settings.DATA == false) {
                settings.DATA = true;
            }
            saveSettings();
        case 13: //n
        case 10: //r
            break;
        case 42:
            missionStatus = 1;
            fps = 850;
            sendSerialDataln("Starting CountDown");
            break;
        case 33: //!
            missionStatus = 0;
            countdown = 20;
            stats.missionTime = millis() - clock;
            sendSerialDataln("DATA LOCK");
            saveStats();
            fps = 1000;
            break;
        case 'u':
        case 'U':
            sendSerialData("Altitude:    ");
            sendSerialData(String(curAlti));
            sendSerialDataln(" m");
            sendSerialData("Ali Change:    ");
            sendSerialData(String(curAlti - baseline));
            sendSerialDataln(" m");
            sendSerialDataln("");
            break;
        default:
            //Invalid option
            sendSerialData("\n");
            sendSerialDataln("error: invalid option");
            sendSerialDataln("Enter h or ? for help");
            break;
        }

    }

    updateStatusLED(1);
    while (1) {
        if (millis() - time > fps) {
            break;
        }
    }
    updateStatusLED(0);



    sendSerialData("Time: ");
    sendSerialDataln(String(millis() - time));

}

void printStatus() {
    sendSerialData("\n");

    sendSerialData("Debug Status: ");
    sendSerialDataln(String(settings.DEBUG));

    sendSerialData("2.4GHz Status: ");
    sendSerialDataln(String(settings.DOWNLINK));

    sendSerialData("Data Status: ");
    sendSerialDataln(String(settings.DATA));
    sendSerialData("\n");

}

void saveSettings() {

    EEPROM_writeAnything(ESET, settings);

    sendSerialDataln("Settings Saved");
}

void saveStats() {

    EEPROM_writeAnything(0, stats);

    sendSerialDataln("New Stat Saved");
}

void printHelp() {
    sendSerialData("\n");
    sendSerialDataln("Commands:");
    sendSerialData("d/D - Debug Output On/Off - ");
    sendSerialDataln(String(settings.DEBUG));
    sendSerialData("x/X - 2.4 GHz Downlink On/Off - ");
    sendSerialDataln(String(settings.DOWNLINK));
    sendSerialDataln("c/C - Clear Stats");
    sendSerialDataln("s/S - Print Current Settings");
    sendSerialDataln("h/? - This Help Message");
    sendSerialData("\n");
}

void sendSerialData(String message) {
    if (settings.DEBUG) {
        Serial.print(message);
    }
    if (settings.DOWNLINK) {
        mySerial.print(message);
    }

}
void sendSerialDataln(String message) {
    if (settings.DEBUG) {
        Serial.println(message);
    }
    if (settings.DOWNLINK) {
        mySerial.println(message);
    }

}

void updateStatusLED(int state) {
    switch (state) {
    case 1:
        if (missionStatus == 0) {
            digitalWrite(red, HIGH);
            digitalWrite(blue, LOW);
            digitalWrite(green, LOW);
        }
        if (missionStatus == 1) {
            digitalWrite(red, LOW);
            digitalWrite(blue, HIGH);
            digitalWrite(green, LOW);
        }
        if (missionStatus == 2) {
            digitalWrite(red, LOW);
            digitalWrite(blue, LOW);
            digitalWrite(green, HIGH);
        }
        break;
    case 0:
        if (missionStatus == 0) {
            digitalWrite(red, HIGH);
            digitalWrite(blue, LOW);
            digitalWrite(green, LOW);
        } else {
            digitalWrite(blue, LOW);
            digitalWrite(green, LOW);
            digitalWrite(red, LOW);
        }
        break;
    }



}
