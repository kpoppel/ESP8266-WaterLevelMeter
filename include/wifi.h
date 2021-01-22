#include <Arduino.h>

/*  My WiFi connection functions.
 *  Setup variables like these:
 *   const char *ssid = "<..>";
 *   const char *password = "<...>";
 *   const uint8_t wifi_bssid[6] = {<>, <>, <>, <>, <>, <>};
 *   const uint8 wifi_channel = <>;
 *   const uint8_t wifi_address[4] = {192, 168, 0, 2};
 *   const uint8_t wifi_gateway[4] = {192, 168, 0, 1};
 *   const uint8_t wifi_subnet[4] = {255, 255, 255, 0};
 *   const uint8_t wifi_dns1[4] = {8, 8, 8, 8};
 *   const uint8_t wifi_dns2[4] = {8, 8, 4, 4};
 * 
 * Call wifi_setup_static_ip first, then wifi_connect if network scan is preferable, or just the wifi_fast_connect.
 * If you have more access points with the same SSID (if there are extenders in the house), scanning for the best one
 * can be an advantage.  But once the device has found its place to live, register the best BSSID and channel and
 * just go with the fast connect because it is faster and will use less power on e.g. a battery powered system.
 * 
 * The wifi-wait_for_connection() function must be called before transmitting or receiving data.  It waits for
 * connection, then returns a status.  1 is an error condition, 0 is connection success.
 * For example: To shorten on-time, begin the connection immediately after boot, the do everything which needs to
 * be done without WiFi, then call the function, and transmit data.
 * 
 * Results here is  ~1.2 seconds to connect to a specific access point with fixed IP, and ~3.3 seconds when using DHCP.
 */

void wifi_setup_static_ip(const uint8_t *address, const uint8_t *gateway, const uint8_t *subnet, const uint8_t *dns1, const uint8_t *dns2);
void wifi_connect(const char *ssid, const char *password);
void wifi_fast_connect(const char *ssid, const char *password, const uint8 channel, const uint8_t bssid[6]);
uint8 wifi_wait_for_connection();
