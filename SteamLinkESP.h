#ifndef STEAMLINKESP_H
#define STEAMLINKESP_H

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ESP32
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#endif

#include <math.h>

#if defined(ESP8266) or defined(ESP32)
#include <SPI.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <SteamLinkGeneric.h>
#include <SteamLink.h>

#define SL_ESP_DEFAULT_TOPIC_LEN 100
#define SL_ESP_MAX_MESSAGE_LEN 255


#define WIFI_WAITSECONDS 15

struct SteamLinkWiFi {
  const char *ssid;
  const char *pass;
};

struct SteamLinkESPConfig {
  bool encrypted;
  uint8_t *key; 
  struct SteamLinkWiFi  *creds;
  const char *sl_server;
  uint16_t sl_serverport;
  const char *sl_username;
  const char *sl_conid;
  const char *sl_key;
  const char *sl_server_fingerprint;
};


///////////////////////////////////////////////////////////////////////////
//   TODO: STEAMLINK CREDENTIALS
///////////////////////////////////////////////////////////////////////////
/*
#ifndef VER

// WiFi Creds
struct Credentials {
  char *ssid;
  char *pass;
};

// WiFI credential entries
struct Credentials creds[] =
  {
    { "SSID1", "PPP" },
    { "SSID2", "PPP" },
    { "SSID3", "PPP"}
  };

// numer of secods to wait for a WiFi connection
#define WIFI_WAITSECONDS 15

// MQTT broker
#define SL_SERVER      "mqtt.steamlink.net"
#define SL_SERVERPORT  8883                   // 8883 for MQTTS

// SHA1 fingerprint for mqtt.steamlink.net's SSL certificate
const char* sl_server_fingerprint = "E3 B9 24 8E 45 B1 D2 1B 4C EF 10 61 51 35 B2 DE 46 F1 8A 3D";

// Bridge Token
#define SL_USERNAME    "UUUU"
#define SL_CONID       "CON"
#define SL_KEY         "PPPP"

#endif
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
*/

class SteamLinkESP : public SteamLinkGeneric {
  public:

  /// constructor
  SteamLinkESP(SL_NodeCfgStruct *config);

  virtual void init(void *vconf, uint8_t config_length);

  /// send
  virtual bool driver_send(uint8_t* packet, uint8_t packet_length, uint32_t slid);

  /// receive
  virtual bool driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid);


 private:

  // config info
  struct SteamLinkESPConfig *_conf;
  struct SteamLinkWiFi *_creds;
  
  char _pub_str[SL_ESP_DEFAULT_TOPIC_LEN];
  char _sub_str[SL_ESP_DEFAULT_TOPIC_LEN];

  WiFiClientSecure _client;
  Adafruit_MQTT_Client* _mqtt;

  Adafruit_MQTT_Publish* _pub;
  Adafruit_MQTT_Subscribe* _sub;

  void wifi_connect();
  bool mqtt_connect();

  void create_pub_str(char* topic, uint32_t slid);
  void create_sub_str(char* topic, uint32_t slid);

  static void _sub_callback(char* data, uint16_t len);

  // TODO: use queue?
  static uint8_t driverbuffer[SL_ESP_MAX_MESSAGE_LEN];
  static uint8_t rcvlen;
  static bool available;
};

#endif // ESP8266
#endif // ifdef STEAMLINK_ESP_H
