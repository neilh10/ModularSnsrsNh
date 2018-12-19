/*
 * This example shows how to schedule some work at
 * regular intervals.
 *
 * This example is a bit more advanced. It uses the WDT as
 * an interrupt to wake up from deep sleep. During the wake
 * up it does the RTCTimer.update so that it can do scheduled
 * tasks. The RTC continues to run during the deep sleep so the
 * timing remains accurate. And at the same time the power
 * consumption is as low as possible.
 * 
 * Notice that the "epoch" of the RTC is used and the time units
 * are in seconds.  (The Timer library works with millis() so its
 * time units are milliseconds.
 */

// We need these includes to program the WDT
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <stdarg.h>
#include <wire.h>
#include <Sodaq_DS3231.h>
#include <RTCTimer.h>
#include <EnableInterrupt.h>// To handle external and pin change interrupts

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define Ser1 SerialUSB
#else
  #define Ser1 Serial
#endif
const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

RTCTimer timerEvt; // Instantiate the timer

//const int led = 6;
const int8_t greenLED = 8;        // MCU pin for the green LED (-1 if not applicable)
const int8_t redLED = 9;          // MCU pin for the red LED (-1 if not applicable)
//const int8_t buttonPin = 21;      // MCU pin for a button to use to enter debugging mode  (-1 if not applicable)
const int8_t _mcuWakePin = A7;   

bool hz_flag;  //1sec list
bool tmr_flag;

const uint8_t timeZone_def= -8;
void printTime() 
{
  DateTime nowUtc = rtc.now(); //timeZone_def*60 get the current date-time
  uint32_t ts = nowUtc.getEpoch()+(timeZone_def*60);
  DateTime now(ts);
  //if (old_ts == 0 || old_ts != ts) 
  {
   // Serial.print(now.year(), DEC);
    //Serial.print('/');
    //Serial.print(now.month(), DEC);
    //Serial.print('/');
    Serial.print(now.date(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    //Serial.print(' ');
    //Serial.print(weekDay[now.dayOfWeek()]);
    //Serial.println();
    Serial.print(" Epoch:"); 
    Serial.print(ts, DEC);
    Serial.println();
  }

}
// Set up the Interrupt Service Request for waking
// In this case, we're doing nothing, we just want the processor to wake
// This must be a static function (which means it can only call other static funcions.)
void wakeISR(void)
{
    // MS_DBG(F("Clock interrupt!"));
    //tmr_flag=true;
}
// Sets up the sleep mode

void setupSleep(void)
{
    // Set the pin attached to the RTC alarm to be in the right mode to listen to
    // an interrupt and attach the "Wake" ISR to it.
    pinMode(_mcuWakePin, INPUT_PULLUP);
    enableInterrupt(_mcuWakePin, wakeISR, CHANGE);

    // Unfortunately, because of the way the alarm on the DS3231 is set up, it
    // cannot interrupt on any frequencies other than every second, minute,
    // hour, day, or date.  We could set it to alarm hourly every 5 minutes past
    // the hour, but not every 5 minutes.  This is why we set the alarm for
    // every minute and use the checkInterval function.  This is a hardware
    // limitation of the DS3231; it is not due to the libraries or software.
    rtc.enableInterrupts(EverySecond);

    // Set the sleep mode
    // In the avr/sleep.h file, the call names of these 5 sleep modes are:
    // SLEEP_MODE_IDLE         -the least power savings
    // SLEEP_MODE_ADC
    // SLEEP_MODE_PWR_SAVE
    // SLEEP_MODE_STANDBY
    // SLEEP_MODE_PWR_DOWN     -the most power savings
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}
void setup()
{
  pinMode(greenLED, OUTPUT);

  Ser1.begin(115200);
  while(!Ser1){delay(10);};
  Ser1.println("---PwrMon Readings");
  Ser1.println(compile_date);
  Ser1.println(file_name); //Dir and filename
  Wire.begin();
  rtc.begin();

  Serial.flush();

  // Do the work every 5 seconds
  timerEvt.every(5, toggleLed);

  // Instruct the RTCTimer how to get the current timestamp
  timerEvt.setNowCallback(getNow);
//#define TIMER_WTCHDG 1
#ifdef TIMER_WTCHDG
  //setupWatchdog();
#else  //TIMER_WTCHDG
#endif //TIMER_WTCHDG
  interrupts();
  tmr_flag=true; //1st time thru
}

void loop()
{
  timerEvt.update();
  if (hz_flag) {
    Ser1.write('s');
    hz_flag = false;
    wdt_reset();
    WDTCSR |= _BV(WDIE);
  }
  if (tmr_flag) {
    //Ser1.write('T');
    tmr_flag = false;
    printTime() ;
    //delay(10);//allow chars out
  }


  systemSleep();
}
#if 1 //defined(TIMER_WTCHDG)
void toggleLed(uint32_t ts)
{
  static bool on;

  on = !on;
  digitalWrite(greenLED, on);
  tmr_flag=true;
}

/*
 * Return the current timestamp
 *
 * This is a general purpose wrapper function to get the current timestamp.
 * It can also be used for timer.setNowCallback
 */
uint32_t getNow()
{
  return rtc.now().getEpoch();
}

#endif // TIMER_WTCHDG


//######### watchdog and system sleep #############
void systemSleep()
{
#if !defined(TIMER_WTCHDG)
  // Make sure the RTC is still sending out interrupts
  rtc.enableInterrupts(EverySecond);

  // Clear the last interrupt flag in the RTC status register
  // The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();

  // Make sure we're still set up to handle the clock interrupt
  pinMode(_mcuWakePin, INPUT_PULLUP);
  enableInterrupt(_mcuWakePin, wakeISR, CHANGE);
  // Stop any I2C connections
  // This function actually disables the two-wire pin functionality and
  // turns off the internal pull-up resistors.
  // It does NOT set the pin mode!
  Wire.end();
  // Now force the I2C pins to LOW
  // I2C devices have a nasty habit of stealing power from the SCL and SDA pins...
  // This will only work for the "main" I2C/TWI interface
  pinMode(SDA, OUTPUT);  // set output mode
  pinMode(SCL, OUTPUT);
  digitalWrite(SDA, LOW);  // Set the pins low
  digitalWrite(SCL, LOW);

  // Temporarily disables interrupts, so no mistakes are made when writing
  // to the processor registers
  //noInterrupts();  - see LoggerBase.cpp#441
#endif //!defined(TIMER_WTCHDG)
  ADCSRA &= ~_BV(ADEN);         // ADC disabled

  /*
  * Possible sleep modes are (see sleep.h):
  #define SLEEP_MODE_IDLE         (0)
  #define SLEEP_MODE_ADC          _BV(SM0)
  #define SLEEP_MODE_PWR_DOWN     _BV(SM1)
  #define SLEEP_MODE_PWR_SAVE     (_BV(SM0) | _BV(SM1))
  #define SLEEP_MODE_STANDBY      (_BV(SM1) | _BV(SM2))
  #define SLEEP_MODE_EXT_STANDBY  (_BV(SM0) | _BV(SM1) | _BV(SM2))
  */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_mode();

  ADCSRA |= _BV(ADEN);          // ADC enabled

  // Re-start the I2C interface
  pinMode(SDA, INPUT_PULLUP);  // set as input with the pull-up on
  pinMode(SCL, INPUT_PULLUP);
  Wire.begin();

}

#if 0
// The watchdog timer is used to make timed interrupts
#if 0
// Both WDE and WDIE are set!!
// Note from the doc: "Executing the corresponding interrupt
// vector will clear WDIE and WDIF automatically by hardware
// (the Watchdog goes to System Reset Mode)
#define my_wdt_enable(value)   \
__asm__ __volatile__ (  \
    "in __tmp_reg__,__SREG__" "\n\t"    \
    "cli" "\n\t"    \
    "wdr" "\n\t"    \
    "sts %0,%1" "\n\t"  \
    "out __SREG__,__tmp_reg__" "\n\t"   \
    "sts %0,%2" "\n\t" \
    : /* no outputs */  \
    : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
      "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
      "r" ((uint8_t) (((value & 0x08) ? _WD_PS3_MASK : 0x00) | \
          _BV(WDE) | _BV(WDIE) | (value & 0x07)) ) \
    : "r0"  \
)
#else
// Only WDIE is set!!
#define my_wdt_enable(value)   \
__asm__ __volatile__ (  \
    "in __tmp_reg__,__SREG__" "\n\t"    \
    "cli" "\n\t"    \
    "wdr" "\n\t"    \
    "sts %0,%1" "\n\t"  \
    "out __SREG__,__tmp_reg__" "\n\t"   \
    "sts %0,%2" "\n\t" \
    : /* no outputs */  \
    : "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
      "r" (_BV(_WD_CHANGE_BIT) | _BV(WDE)), \
      "r" ((uint8_t) (((value & 0x08) ? _WD_PS3_MASK : 0x00) | \
          _BV(WDIE) | (value & 0x07)) ) \
    : "r0"  \
)
#endif

void setupWatchdog()
{
  my_wdt_enable(WDTO_1S);
}

//################ interrupt ################
ISR(WDT_vect)
{
  hz_flag = true;
}
#endif //00