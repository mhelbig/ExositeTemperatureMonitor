/*
 Exosite Arduino Basic Temp Monitor 2 (updated to use Exosite library)
  
 This sketch shows an example of sending data from a connected
 sensor to Exosite. (http://exosite.com) Code was used from various
 public examples including the Arduino Ethernet examples and the OneWire
 Library examples found on the Arduino playground. 
 (OneWire Lib credits to Jim Studt, Tom Pollard, Robin James, and Paul Stoffregen)
  
 This code keeps track of how many milliseconds have passed
 and after the user defined REPORT_TIMEOUT (default 60 seconds)
 reports the temperature from a Dallas Semi DS18B20 1-wire temp sensor.
 The code sets up the Ethernet client connection and connects / disconnects 
 to the Exosite server when sending data.
  
 Assumptions:
 - Tested with Aruduino 1.0.5
 - Arduino included Ethernet Library
 - Arduino included SPI Library
 - Using Exosite library (2013-10-20) https://github.com/exosite-garage/arduino_exosite_library
 - Using OneWire Library Version 2.0 - http://www.arduino.cc/playground/Learning/OneWire
 - Using Dallas Temperature Control Library - http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_372Beta.zip
 - Uses Exosite's basic HTTP API, revision 1.0 https://github.com/exosite/api/tree/master/data
 --- USER MUST DO THE FOLLOWING ---
 - User has an Exosite account and created a device (CIK needed / https://portals.exosite.com -> Add Device)
 - User has added a device to Exosite account and added a data source with alias 'temp', type 'float'
 
  
 Hardware:
 - Arduino Duemilanove or similiar
 - Arduino Ethernet Shield
 - Dallas Semiconductor DS18B20 1-Wire Temp sensor used in parasite power mode (on data pin 7, with 4.7k pull-up)
 
Version History:
- 1.0 - November 8, 2010 - by M. Aanenson
- 2.0 - July 6, 2012 - by M. Aanenson
- 3.0 - October 25, 2013 - by M. Aanenson - Note: Updated to use latest Exosite Arduino Library

*/
  
#include <MemoryFree.h>  
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Exosite.h>
 
// Pin use
#define ONEWIRE       7  // pin to use for One Wire interface
#define DigitalInput  6  // pin to use to read an On/Off status
#define LED           8  // pin to show status LED

// Set up which Arduino pin will be used for the 1-wire interface to the sensor
OneWire oneWire(ONEWIRE);
DallasTemperature sensors(&oneWire);
 
/*==============================================================================
* Configuration Variables
*
* Change these variables to your own settings.
*=============================================================================*/
String cikData = "24797b6d8157da330799a3bb21eb28b657ceed66";  // <-- FILL IN YOUR CIK HERE! (https://portals.exosite.com -> Add Device)
byte macData[] = {0x90, 0xA2, 0xDA, 0x0D, 0x99, 0x15};        // <-- FILL IN YOUR Ethernet shield's MAC address here.
#define ETHERNET_STARTUP_TIME 3                              // Start-up time in seconds

// User defined variables for Exosite reporting period and averaging samples
#define SENSOR_READ_TIMEOUT 10000 //milliseconds period for reporting data


/*==============================================================================
* End of Configuration Variables
*=============================================================================*/

class EthernetClient client;
Exosite exosite(cikData, &client);

void setup() {
  pinMode(DigitalInput, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Boot");

  Serial.print("freeMemory = ");
  Serial.println(freeMemory());

  // Start up the OneWire Sensors library
  sensors.begin();
  delay(1000);
 
  Serial.println("Ethernet.begin initiated");
  Ethernet.begin(macData);
  Serial.println("Ethernet.begin completed");
  // wait some time for Ethernet connection

  Serial.println("Waiting Ethernet startup time");
  for (int i=0; i<ETHERNET_STARTUP_TIME; i++)
  {
    Serial.print(".");
    LEDon();
    delay (500);
    LEDoff();
    delay (500);
  }
  Serial.println("Ethernet startup time complete");
}
 
void loop() {
  static unsigned long sendPrevTime = 0;
  static unsigned long sensorPrevTime = 0; 
  static float tempF;
  int InputStatus;
  int ErrorCount = 0;
  char buffer[10];
  String readParam = "";
  String writeParam = "";
  String returnString = "";  
   
  Serial.print("."); // marching dots show that it's running
  LEDon();            // also flash the LED to show that it's running
  delay(30);
  LEDoff();
   
 // Read sensors and send data to Exosite every defined timeout period
  if (millis() - sensorPrevTime > SENSOR_READ_TIMEOUT) 
  {
    sensors.requestTemperatures(); // Send the command to get temperatures
    tempF = DallasTemperature::toFahrenheit(sensors.getTempCByIndex(0));
    Serial.println();
    Serial.print("Temperature reading: ");
    Serial.println(tempF);

    InputStatus = !digitalRead(DigitalInput);
    Serial.print("Input Status: ");
    Serial.println(InputStatus);
    
    sensorPrevTime = millis();

    Serial.println("Building writeParam String");
    readParam = "";        //nothing to read back at this time e.g. 'control&status' if you wanted to read those data sources
    writeParam = "Temperature="; //parameters to write e.g. 'temp=65.54' or 'temp=65.54&status=on'
    String tempValue = dtostrf(tempF, 1, 2, buffer); // convert float to String, minimum size = 1, decimal places = 2
    writeParam += tempValue;    //add converted temperature String value
    
    writeParam += "&Input="; //add add parameter for Digital Input status
    writeParam += String(InputStatus, DEC);
    
    writeParam += "&Errors="; //add add parameter for Digital Input status
    writeParam += String(ErrorCount, DEC);
    
    Serial.print("writeParam String being sent to Exosite: ");
    Serial.println(writeParam);

    LEDon();
    if ( exosite.writeRead(writeParam, readParam, returnString))
    {
      if (returnString != "")
      {
        Serial.println("Response:");
        Serial.println(returnString);
      }
      else
      {
        Serial.println("Data written successfully to Exosite");
      }
    }
    else
    {
      ErrorCount++;
      Serial.print("Exosite Error #");
      Serial.println(ErrorCount);
    }
  }
  
  LEDoff();
  delay(1000); //slow down loop
}

void LEDon(void)
{
  digitalWrite(LED,1);
}

void LEDoff(void)
{
  digitalWrite(LED,0);
}


