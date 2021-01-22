#include <WiFiClient.h>
#include <PubSubClient.h>
#include "config.h"
#include "mqtt.h"

WiFiClient espClient;
PubSubClient client(espClient);
String mqtt_message;

//char mqtt_message[1000];

PubSubClient mqtt_client()
{
    return client;
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.printf("Message arrived in topic '%s':\r\n", topic);
    for (unsigned int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

uint8_t mqtt_connect(const char *mqttServer, const int mqttPort, const char *mqttUser, const char *mqttPassword)
{
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    while (!client.connected())
    {
        Serial.print("Connecting to MQTT...");

        if (client.connect("ESP8266Client", mqttUser, mqttPassword))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.printf("failed with state: %d\r\n", client.state());
            return 1;
        }
    }
    return 0;
}

void mqtt_publish(const char *channel)
{
    // Add curly brace to the message and transmit
    mqtt_message = "{" + mqtt_message;
    mqtt_message += "}";
    client.publish(channel, mqtt_message.c_str());
}

void mqtt_subscribe(const char *channel)
{
    client.subscribe(channel);
}

void mqtt_client_loop()
{
    client.loop();
}

void mqtt_disconnect()
{
    client.disconnect();
}

void __mqtt_message_begin(const char* key)
{
    if(mqtt_message.length() > 0)
        mqtt_message += ",";

    mqtt_message += "\"";
    mqtt_message += key;
    mqtt_message += "\":\"";
}

void mqtt_add_message(const char* key, const char* value)
{
    __mqtt_message_begin(key);
    mqtt_message += value;
    mqtt_message += "\"";
}

void mqtt_add_message(const char* key, int value)
{
    __mqtt_message_begin(key);
    mqtt_message += value;
    mqtt_message += "\"";
}

void mqtt_add_message(const char* key, float value)
{
    static char value_str[15];
    dtostrf(value, 5, 2, value_str);

    __mqtt_message_begin(key);
    mqtt_message += value_str;
    mqtt_message += "\"";
}
