#ifndef STEAMLINKPAHO_H
#define STEAMLINKPAHO_H

#ifdef UNIX

#include <math.h>

#include <SteamLinkGeneric.h>
#include <SteamLink.h>

#include <memory.h>
#include "MQTTClient.h"
#define DEFAULT_STACK_SIZE -1
#include "linux.cpp"

#define SL_PAHO_DEFAULT_TOPIC_LEN 100
#define SL_PAHO_MAX_MESSAGE_LEN 255

struct SteamLinkPahoConfig {
  bool encrypted;
  uint8_t *key; 
  const char *sl_server;
  uint16_t sl_serverport;
  const char *sl_username;
  const char *sl_conid;
  const char *sl_key;
  const char *sl_server_fingerprint;
};

class SteamLinkPaho : public SteamLinkGeneric {
  public:

  /// constructor
  SteamLinkPaho(SL_NodeCfgStruct *config);

  virtual void init(void *vconf, uint8_t config_length);

  /// send
  virtual bool driver_send(uint8_t* packet, uint8_t packet_length, uint32_t slid);

  /// receive
  virtual bool driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid);


 private:

  // config info
  struct SteamLinkPahoConfig *_conf;
  
  char _pub_str[SL_PAHO_DEFAULT_TOPIC_LEN];
  char _sub_str[SL_PAHO_DEFAULT_TOPIC_LEN];

  IPStack* _ipstack;
  MQTT::Client<IPStack, Countdown>* _client;
  MQTTPacket_connectData _data = MQTTPacket_connectData_initializer;    
  
  bool mqtt_connect();

  void create_pub_str(char* topic, uint32_t slid);
  void create_sub_str(char* topic, uint32_t slid);

  static void _sub_callback(MQTT::MessageData& md);

  // TODO: use queue?
  static uint8_t driverbuffer[SL_PAHO_MAX_MESSAGE_LEN];
  static uint8_t rcvlen;
  static bool available;
};

#endif // ifdef UNIX
#endif // ifdef STEAMLINKPAHO_H
