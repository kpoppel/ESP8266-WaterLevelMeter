#include <ESP8266WebServer.h>
#include "config.h"
#include "index.h"

#define LED D0

ESP8266WebServer server(config::http_port);

/* Webserver functions */
void handleRoot()
{
    // The static webpage is stored in PROGMEM (Flash).  To fetch the data from PROGMEM the compiler
    // needs to know it is there and not in SRAM.
    __FlashStringHelper *s = (__FlashStringHelper *)webpage;
    server.send(200, "text/html", s);

    // Debugging - dump the page to console:
    Serial.println(s);
    // Debugging - just send a simple text message:
    //  server.send(200,"text/plain","Hello WOrld");
}

void sensor_data()
{
    int a = analogRead(A0);
    int temp = a / 4.35;
    String sensor_value = String(temp);
    server.send(200, "text/plane", sensor_value);
}

void led_control()
{
    String state = "OFF";
    String act_state = server.arg("state");
    if (act_state == "1")
    {
      digitalWrite(LED, HIGH); //LED ON
      state = "ON";
    }
    else
    {
      digitalWrite(LED, LOW); //LED OFF
      state = "OFF";
    }

    server.send(200, "text/plane", state);
}

void http_begin()
{
    // Web server routing
    server.on("/", handleRoot);
    server.on("/led_set", led_control);
    server.on("/adcread", sensor_data);
    server.begin();
}

void http_loop()
{
    server.handleClient();
}
