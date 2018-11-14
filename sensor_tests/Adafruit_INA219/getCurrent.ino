//https://github.com/adafruit/Adafruit_INA219/blob/master/examples/getcurrent/getcurrent.ino
#include <stdio.h>
#include <stdarg.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
//#include "Sodaq_DS3231.h"
#include <Sodaq_DS3231.h>
//#include "TimerOne.h"
//#include <time.h>
#include <timeLib.h>

Adafruit_INA219 ina219;
const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;
#if 0
void p(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}
#endif //0

  #define PST_OFFSET_mS 8*60*60*1000
  uint32_t currentEpochSodaqTimePST_mS = 0;

const unsigned long mSecInMinute =  60000;
const unsigned long mSecInHour  = 3600000;

char const digit[] = "0123456789";
//Fast uint To Ascii with field size
char* utoas(int i, char b[],int fill,char sz){
    char* p = b+sz;

    memset(b,fill,sz);
    *p = '\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}

void printDebugTime(void) {
  unsigned long TimeNow_mS = millis();
  unsigned long TimeNow_sec = TimeNow_mS/1000;
  //unsigned long hours_mS;     //0-23*60,000
  //unsigned long hours_rem_mS; //0-23*60,000
  //unsigned int minutes_mS; //0-59,000
  unsigned int millisecs;  //0-999
  byte seconds; //0-59
  byte minutes; //0-59
  byte hours;  //0-23
  char buff[10];//Max Size 9999
  //setTime(TimeNow_mS);

 
  hours =    hour(TimeNow_sec);//   TimeNow_mS/mSecInHour;
  //hours_rem_mS =TimeNow_mS-hours*mSecInHour;
  minutes =  minute(TimeNow_sec);//   hours_rem_mS/(mSecInMinute);
  //minutes_mS = minutes*mSecInMinute;
  seconds =  second(TimeNow_sec);//(hours_rem_mS-minutes_mS)/1000;
  millisecs =TimeNow_mS-(hours*mSecInHour+minutes*mSecInMinute+seconds*1000);
  if (millisecs>999) millisecs=999;

  Serial.print(utoas(hours,buff,'0',2));
  //Serial.print(hours);    
  Serial.print(":");
  //Serial.print(minutes);
  //Serial.print("/");
  Serial.print(utoas(minutes,buff,'0',2));

  Serial.print(":"); 

  Serial.print(utoas(seconds,buff,'0',2)); 
  //Serial.print("/"); 
  //Serial.print(seconds); 
  Serial.print("."); 
  Serial.print(utoas(millisecs,buff,'0',3)); 
  Serial.print(" ");
}

// This gets the current epoch time (unix time, ie, the number of seconds
// from January 1, 1970 00:00:00 UTC) and corrects it for the specified time zone
#define EPOCH_TIME_OFF 946684800
DateTime dtFromEpoch(uint32_t epochTime)
{
    DateTime dt(epochTime - EPOCH_TIME_OFF);
    return dt;
}

// This converts a date-time object into a ISO8601 formatted string
const int8_t _timeZone = 0;
String formatDateTime_ISO8601_dt(DateTime& dt)
{
    // Set up an inital string
    String dateTimeStr;
    // Convert the DateTime object to a String
    dt.addToString(dateTimeStr);
    dateTimeStr.replace(" ", "T");
    String tzString = String(_timeZone);
    if (-24 <= _timeZone && _timeZone <= -10)
    {
        tzString += F(":00");
    }
    else if (-10 < _timeZone && _timeZone < 0)
    {
        tzString = tzString.substring(0,1) + F("0") + tzString.substring(1,2) + F(":00");
    }
    else if (_timeZone == 0)
    {
        tzString = F("Z");
    }
    else if (0 < _timeZone && _timeZone < 10)
    {
        tzString = "+0" + tzString + F(":00");
    }
    else if (10 <= _timeZone && _timeZone <= 24)
    {
        tzString = "+" + tzString + F(":00");
    }
    dateTimeStr += tzString;
    return dateTimeStr;
}


// This converts an epoch time (unix time) into a ISO8601 formatted string
String formatDateTime_ISO8601_u32(uint32_t epochTime)
{
    // Create a DateTime object from the epochTime
    DateTime dt = dtFromEpoch(epochTime);
    return formatDateTime_ISO8601_dt(dt);
}

void setup(void) 
{
  Serial.begin(115200);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }

  uint32_t currentFrequency;
    
  //Serial.println("Una219 begin!");
  Serial.print("---Ina219 ");
  Serial.println(compile_date);
  Serial.println(file_name); //Dir and filename
  //Serial.print(BUILD_TIMESTAMP); 
  //Serial.println(") ");

  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  ina219.begin();
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();

  Serial.println("Reporting Power and current with INA219 0-400mA ...");

  currentEpochSodaqTimePST_mS = rtc.now().getEpoch()-PST_OFFSET_mS;
  Serial.print(F("Current RTC time is: "));
  //Serial.print(currentEpochSodaqTime);
  Serial.println(formatDateTime_ISO8601_u32(currentEpochSodaqTimePST_mS));
}

void loop(void) 
{
  //float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  //float loadvoltage = 0;
  float power_mW = 0;
  char sBuf[16];

  //shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  //power_mW = ina219.getPower_mW();
  //loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  //Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
  //Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
  //Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
  //Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
  //Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
  //Serial.println("");

  printDebugTime();
  Serial.print(F("mA: "));  Serial.print(dtostrf(current_mA,6,2,sBuf));
  Serial.print(F(", V: ")); Serial.println(busvoltage,3);
  //Serial.print(F(", mW: "));Serial.println(dtostrf(power_mW,4,0,sBuf));

  delay(1994);
}
 
