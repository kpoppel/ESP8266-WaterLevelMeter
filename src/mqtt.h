#include <Arduino.h>

uint8_t mqtt_connect(const char* mqttserver, const int mqttPort, const char* mqttUser, const char* mqttPassword);
void mqtt_publish(const char *channel);
void mqtt_subscribe(const char *channel);
void mqtt_client_loop();
void mqtt_disconnect();
void mqtt_add_message(const char* key, const char* value);
void mqtt_add_message(const char* key, int value);
void mqtt_add_message(const char* key, float value);