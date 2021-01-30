/*
 * ESP8266 code for ESP8266 webserver version of the water level metering software
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ds3231.h>
#include "config.h"
#include "rtc.h"
#include "wifi.h"
#include "http.h"
#include "mqtt.h"

#define LED D4
#define TRIG D5
#define ECHO D6

// Use the ADC internally to measure VCC
ADC_MODE(ADC_VCC);

unsigned long time_awake = 0;

void debug_print_time(const char* message)
{
    time_awake = millis();
    Serial.print(message);
    Serial.println(time_awake);
}

void go_to_sleep()
{
    /* All the sleep logic.  3 modes have been used throughout development:
     * - simulated sleep
     *   This mode uses not sleep really. It just wais for a while before resetting the ESP and start over.
     * - deep_sleep
     *   This mode uses the deepsleep feature of the ESP8266.  Lower power use, but not zero.
     *   D0 must connect to RST for this to work.
     * - rtc_sleep
     *   Using external circuit to cut power. When on USB power we wait for the alarm to come in,
     *   then reset the ESP as if power just came on.
     */
    if(config::use_rtcsleep)
    {
        Serial.println("ESP8266 triggering power-off mode");
        DS3231_clear_a2f();
        // Here the system simply looses power... see you later

        // Never executed when running on battery. On USB power this runs too.
        int seconds = 0;
        while(!DS3231_triggered_a2()) {
            Serial.printf(" -> wait for alarm2 triggered - %d s\r\n", seconds);
            delay(5000);
            seconds += 5;
        }
        Serial.println(" -> alarm2 triggered - reset and start over");
        ESP.reset();
    }
    else if (config::use_deepsleep) {
        Serial.println("ESP8266 in deep sleep mode");
        ESP.deepSleep(config::sleepTimeuS);
        // Bye.  Program will restart after deepsleep.
    }
    else
    {
        // Simulated sleep - only for test. No low-power to be seen here.
        Serial.println("ESP8266 triggering simulated sleep/power-down mode");
        // Turn off board LED - just to see how long the thing is really on
        digitalWrite(LED, HIGH);

        if(config::debug_profile_time) debug_print_time("Before shutdown: ");

        // Wait before simulating waking up again.
        delay(10000);
        // Simulate cold start
        ESP.reset();
    }
    // We never get here. If we did the watchdog will cause a reset.
    while(1) {}
}
/* Setup scenarios I may want to do someday:
 *  1) First setup i.e. power-on setup:
 *     Stay awake for some time waiting for a message to arrive via MQTT.
 *     If a message arrive interpret it and act accordingly.
 *     Currently: ANY message received will keep the device on forever.
 *     Planned messages:
 *       "webserver: {on, off}" - stay awake and start/stop webserver
 *       "restart" - restart the ESP
 *     Requirement: Save somewhere this is a first power-on event
 *  2) Sleep reboot: Do the measurement and go to sleep. (standard mode)
 * 
 * Otherwise:
 *   If ESP8266 module pin D5 is pulled LOW at start, go into 'dev' mode and stay awake.
 *   
*/

float sonar_ping()
{
    // Ping using the ultrasonic sensor to measure distance.
    // Give the sensor at least 60ms to recover if several pings are used.
    float duration;

    // Initialise the pins for the ultrasound sensor
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
  
    digitalWrite(ECHO, LOW);
    duration = pulseIn(ECHO, HIGH);   // measure the echo time (μs)
    return (duration / 2.0) * 0.0343; // convert echo time to distance (cm)
}

void setup()
{
    if(config::debug_profile_time) debug_print_time("Before setup: ");

    // Development mode - Turn on board LED while we are active
    pinMode(LED, OUTPUT);
    //digitalWrite(LED, LOW);  // turn on
    digitalWrite(LED, HIGH); // turn off

    Serial.begin(115200);

    // Fastest way to connect to wifi: static IP and pre-selected base station.
    wifi_setup_static_ip(config::wifi_address, config::wifi_gateway, config::wifi_subnet, config::wifi_dns1, config::wifi_dns2);

    // DS3231 RTC initialisation: Initialise the SDA/SCL pins (D1/D2))
    Wire.begin();
    DS3231_init(DS3231_CONTROL_BBSQW | DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE);

    // ****************************************
    // If the RTC clock was reset to 1970 (due to battery change) we setup
    // the time to match the time of compilation of the software.
    rtc_set_time_once();
    // ****************************************

    // Set the next alarm - we clear the alarm when ready to go to sleep.
    rtc_set_next_alarm();

    if(config::use_webservice)
    {
        http_begin();
    }

    if(config::debug_profile_time) debug_print_time("After setup: ");
}

void loop(void)
{
    /*
     * Do several tings in the main loop:
     * 1 - Check if Wifi got lost (development only, the connect routine will keep trying forever)
     *     Consider though changing it to trying a number of times, then give up until next time slot.
     * 2 - Perform the measurement
     *     TODO: I don't know how accurate it is, or a number of measurements has to be made,
     *     averaging or selecting most common result or whatever.
     * 3 - Send message via MQTT to Home-Assistant
     * 4 - Sleep (several solutions possible, from deepsleep to turning off power using exernal circuit
     */
    // On battery the ultrasound sensor needs some time to start.
    
    if(config::debug_profile_time) debug_print_time("Before measurements: ");

    // Measure battery level
    // Conversion factor from measure battery to ADC measurement is
    // approximately: 0.835 ~= 5/6
    int batt;
    batt = ESP.getVcc();
    batt = (int) (batt * 0.835);

    // Measure the temperature
    float temperature;
    temperature = DS3231_get_treg();

    // Trigger the ultrasound transceiver
    // On battery operation the first 1-2 samples can be zero or inaccurate as
    // something needs to charge first because power just came on.
    // Waiting until after Wifi is up helps.
    // Dump the first 2 samples and use the 3rd (but save and send to MQTT receiver)
//    delay(60);
    float distance;
    distance = sonar_ping();
//    float distance[3];
//    distance[0] = sonar_ping();
//    delay(200);
//    distance[1] = sonar_ping();
//    delay(200);
//    distance[2] = sonar_ping();


    if(config::debug_profile_time) debug_print_time("After measurements: ");

    if(config::debug_profile_time) debug_print_time("Before MQTT transmit: ");

    // Add measurements to MQTT message
    //mqtt_add_message("waterlevel", distance[0]);
    //mqtt_add_message("waterlevel", distance[1]);
    //mqtt_add_message("waterlevel", distance[2]);
    mqtt_add_message("waterlevel", distance);
    mqtt_add_message("vcc", batt);
    mqtt_add_message("temperature", temperature);

    // WiFi draws some current and may cause unstable ADC readings, so connect Wii when needed.
    wifi_fast_connect(config::ssid, config::password, config::wifi_channel, config::wifi_bssid);
    // Can also just scan the network and find the best matching access point (slower):
    //wifi_connect(ssid, password);  // scan for best matching SSID
    if(wifi_wait_for_connection()) {
        // Wifi did not connect. Clear the alarm to power off and wait it out instead.
        go_to_sleep();
    }

    // If webserver should run
    if (config::use_webservice)
      http_loop();

    // Finally wait for connection to WiFi and transmit data, then end the connection.
    if(config::use_mqtt)
    {
        // Publish data
        if(mqtt_connect(config::mqttServer, config::mqttPort, config::mqttUser, config::mqttPassword))
        {
            // Connection failed for some reason. Go to sleep and wai for the next time slot.
            go_to_sleep();
        }
        // Subscribe to a channel (perhaps not needed unless I have a sinister plan of configuring
        // this via MQTT.  Then I need to learn how to store values in non-volatile memory.)
        //mqtt_subscribe(mqttChannel);

        // Transmit data
        mqtt_publish(config::mqttChannel);

        // The loop takes care of all the publishing and subscriptions.
        // If MQTT packets is supposed to be received, the loop must run for a while.
        // Also the MQTT broker must publish messages with the "retained" flag set.
        // This keeps the last message on that channel available forever.
        // So receiving for instance a message on "sensors/waterlevel/config could be used for configuration purposes."
        mqtt_client_loop();
        mqtt_disconnect();
    }

    // Do Serial debugging prints
//    Serial.printf("Water levels are: %.2f, %.2f, %.2f\r\n", distance[0], distance[1], distance[2]);
    Serial.printf("Water level    : %.2f cm\r\n", distance);
    Serial.printf("Battery voltage: %d mV\r\n", batt);
    Serial.printf("Temperature    : %f C\r\n", temperature);

    // End the session
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    if(config::debug_profile_time) debug_print_time("After MQTT transmit: ");

    // Sleep
    go_to_sleep();
}
