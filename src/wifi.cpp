/* Connect to a specific accesspoint the fastest possible
 * Do this using knowledge about the channel and BSSID
 * to omit network scanning for the best SSID.
 * Optionally also fix the IP address etc. and don't use DHCP
 */
//#include "macros.h"
#include <ESP8266WiFi.h>
#include "config.h"

void wifi_station_mode()
{
    //Don't save wifi settings to flash
    WiFi.persistent(false);
    // Go to station mode.  This is not an access point itself.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    //delay(100);
}

void wifi_setup_static_ip(const uint8_t *address, const uint8_t *gateway, const uint8_t *subnet, const uint8_t *dns1, const uint8_t *dns2)
{
    // Using static IP reduces connection time drastically - about 2 seconds.
    IPAddress l_address(address);
    IPAddress l_gateway(gateway);
    IPAddress l_subnet(subnet);
    IPAddress l_dns1(dns1);
    IPAddress l_dns2(dns2);

    WiFi.config(l_address, l_gateway, l_subnet, l_dns1, l_dns2);
}

void wifi_fast_connect(const char *ssid, const char *password, const uint8 channel, const uint8_t bssid[6])
{
    // https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/station-class.html
    // Connect using specific bssid and channel
    WiFi.begin(ssid, password, channel, bssid, true);
}

uint8 wifi_wait_for_connection()
{
    // If the time to connect can be used sensibly, call the waiting for connection loop as late as possible.
    uint16_t retries = 0;

    Serial.print("Wifi: connect");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        //WiFi.printDiag(Serial);

        // Wait up to 4 seconds for Wifi connection
        if(retries++ > 400)
        {
            Serial.println("\r\nWifi: Too many connection attempts. Aborting.");
            return 1;
        }
        delay(10);
    }
    Serial.println("");
    Serial.print("Wifi: IP = ");
    Serial.println(WiFi.localIP());

    return 0;
}

/*  Connect to the strongest accesspoint with the configured SSID
 *  When having several APs with the same SSID, the connection needs
 *  to select a specific one, so let's just go with the strongest signal.
 */
void wifi_connect(const char *ssid, const char *password)
{
    int8 best_rssi = -100;
    int8 best_ssid = -1;

    wifi_station_mode();

    // Scan networks and select the ones with a matching SSID
    Serial.print("Wifi: Scan start. ");
    int n = WiFi.scanNetworks();
    Serial.print(n);
    Serial.println(" network(s) found");
    for (int i = 0; i < n; i++)
    {
#ifdef SERIAL_ON
        Serial.print(i);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" BSSID: ");
        uint8_t *ptr = WiFi.BSSID(i);
        for (int n = 0; n < 6; n++)
        {
            Serial.print(ptr[n]);
            Serial.print(":");
        }
        Serial.print(" CH: ");
        Serial.print(WiFi.channel(i));

        Serial.print(" RSSI: ");
        Serial.print(WiFi.RSSI(i)); //Signal strength in dBm
        Serial.println("dBm");
#endif
        if (strcmp(WiFi.SSID(i).c_str(), ssid) == 0)
        {
            Serial.println("Match on SSID");
            if (WiFi.RSSI(i) > best_rssi)
            {
                best_rssi = WiFi.RSSI(i);
                best_ssid = i;
            }
        }
    }
    Serial.print("Best index: ");
    Serial.println(best_ssid);
    Serial.print("Best rssi: ");
    Serial.println(best_rssi);

    wifi_fast_connect(ssid, password, WiFi.channel(best_ssid), WiFi.BSSID(best_ssid));
    Serial.println("");
}
