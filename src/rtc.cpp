#include <Arduino.h>
#include <ds3231.h>
#include <time.h>
#include "config.h"
#include "mqtt.h"

/* Set next sleep time for the RTC
 * The function uses the global variables 
 *    rtc_sleep_period = [minutes to sleep]
 * set in the config.h file
 */
void rtc_set_next_alarm()
{
    struct ts t;
    struct tm t2;

    // Get current time and calculate new wake-up time.
    DS3231_get(&t);
    t2 = {t.sec, t.min, t.hour,t.mday, t.mon, t.year, t.wday, t.yday, t.isdst}; 
    t2.tm_hour += config::rtc_sleep_h;
    t2.tm_min  += config::rtc_sleep_m;
    mktime(&t2);

    //Serial.printf("\r\nRTC clock:  h:m:s -> %02d:%02d:%02d", t.hour, t.min, t.sec);
    //Serial.printf("\r\nNext alarm: h:m:s -> %02d:%02d:%02d\r\n", t2.tm_hour, t2.tm_min, t2.tm_sec);

    // Add time and alarm to MQTT message
    char buffer[20];
    sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", t.year, t.mon, t.mday, t.hour, t.min, t.sec);
    mqtt_add_message("clock", buffer);
    sprintf(buffer, "%02d:%02d:%02d", t2.tm_hour, t2.tm_min, t2.tm_sec);
    mqtt_add_message("alarm", buffer);

    // set Alarm2.
    DS3231_set_a2( (uint8_t) t2.tm_min, (uint8_t) t2.tm_hour, 0, config::rtc_match);

    // activate Alarm2
    DS3231_set_creg(DS3231_CONTROL_BBSQW | DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE);

    // Once the alarm is set it must be cleared to trigger once again.
    // This is done by calling this function in the wanted place:
    //   DS3231_clear_a2f();

/*
    // Debugging: dump alarm registers to serial.
    int retval;
    char buffer[50];
    for(int addr=0x0B;addr < 0x12;addr++) {
        Serial.print("Address (");
        itoa(addr,buffer,16);
        Serial.print(buffer);
        Serial.print(") :");

        retval=DS3231_get_addr(addr);
        itoa(retval,buffer,2);
        Serial.println(buffer);
    }
*/
}

// Convert compile time to system time 
ts cvt_date(const char *date, const char *time)
{
    char s_month[5];
    int year;
    struct tm t;
    struct ts t2;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(date, "%s %d %d", s_month, &t.tm_mday, &year);
    sscanf(time, "%2d %*c %2d %*c %2d", &t.tm_hour, &t.tm_min, &t.tm_sec);
    // Find where is s_month in month_names. Deduce month value.
    t.tm_mon = (strstr(month_names, s_month) - month_names) / 3 + 1;    
    t.tm_year = year - 1900;    
    mktime(&t);
    t2 = {(uint8_t) t.tm_sec,
          (uint8_t) t.tm_min,
          (uint8_t) t.tm_hour,
          (uint8_t) t.tm_mday,
          (uint8_t) t.tm_mon,
          (int16_t) (t.tm_year + 1900),
          (uint8_t) t.tm_wday,
          (uint8_t) t.tm_yday,
          (uint8_t) t.tm_isdst
        };
    return t2; 
}

void rtc_set_time_once()
{
    // Set the time once to the time of compilation of the software if the year is 1970
    // That would mean the RTC was powered off entirely at some point.
    struct ts t_rtc, t_sw;
    char date[15], time[15];

    // Fetch strings to RAM from flash and convert to ts
    strcpy_P(date, PSTR(__DATE__));
    strcpy_P(time, PSTR(__TIME__));
    t_sw = cvt_date( date, time);

    //Serial.print("SW time before:      ");
    //Serial.printf("%02d/%02d/%02d %02d:%02d:%02d\r\n", t_sw.year, t_sw.mon, t_sw.mday, t_sw.hour, t_sw.min, t_sw.sec);

    // Get current time
    DS3231_get(&t_rtc);

    //Serial.print("RTC time before:     ");
    //Serial.printf("%02d/%02d/%02d %02d:%02d:%02d\r\n", t_rtc.year, t_rtc.mon, t_rtc.mday, t_rtc.hour, t_rtc.min, t_rtc.sec);
    if(t_rtc.year != t_sw.year)
    //if(1) // Force to run (test purposes)
    {
        DS3231_set(t_sw);

        Serial.print("RTC time adjusted:    ");
        DS3231_get(&t_rtc);
        Serial.printf("%02d/%02d/%02d %02d:%02d:%02d\r\n", t_rtc.year, t_rtc.mon, t_rtc.mday, t_rtc.hour, t_rtc.min, t_rtc.sec);
    }
}

