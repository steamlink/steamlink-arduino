// SL_bridge0
// -*- mode: C++ -*-
// bridge for a SteamLink base LoRa network
// https://steamlink.net/

#define VER "11"

#include <SteamLink.h>
#include <SteamLinkESP.h>
#include <SteamLinkLora.h>
#include <SteamLinkBridge.h>

#include "SL_Credentials.h"

#define SL_ID_ESP 0x111
#define SL_ID_LORA 0x110

struct SL_NodeCfgStruct ESPconfig = {
  1,                  // version
	SL_ID_ESP,              // slid
	"ESP_273",          // name
	"ESP Side of Bridge",     // description
	43.43,              // gps_lat
	-79.23,             // gps_lon
	180,                // altitude
	45,                 // max_silence in s
  false,              // battery powered
  1                   // radio_params
};

struct SL_NodeCfgStruct LoRaconfig = {
  1,                  // version
	SL_ID_LORA,              // slid
	"LoRa_272",          // name
	"LoRa Side of Bridge",     // description
	43.43,              // gps_lat
	-79.23,             // gps_lon
	180,                // altitude
	45,                 // max_silence in s
  false,              // battery powered
  1                   // radio_params
};

#if 0
// for Feather M0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#else
// for Adafruit Huzzah breakout
#define RFM95_CS 15
#define RFM95_RST 4
#define RFM95_INT 5
// Blue LED to flash on LoRa traffic
#define LORA_LED 2
// Red LED to flash on mqtt
#define MQTT_LED 0
#endif


void esp_on_receive(uint8_t* buffer, uint8_t size);
void lora_on_receive(uint8_t* buffer, uint8_t size);

SteamLinkESP slesp(&ESPconfig);
SteamLinkLora sllora(&LoRaconfig);
SteamLinkBridge slbridge(&slesp);

/* Packet building */
uint8_t data[100];
int packet_num = 0;

// button state
int8_t bLast, bCurrent = 2;

//
// SETUP
//
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.print(F("!ID SL_bridge0 " VER " slid_ESP: " ));
  Serial.print(SL_ID_ESP, HEX);
  Serial.print(F(" slid_LoRa: "));
  Serial.println(SL_ID_LORA, HEX);

  slesp.init((void *) &sl_ESP_config, sizeof(sl_ESP_config));
  slesp.register_receive_handler(esp_on_receive);
  Serial.println(F("slesp.init done"));

  sllora.set_pins(RFM95_CS, RFM95_RST, RFM95_INT);
  sllora.init((void *) &sl_Lora_config, sizeof(sl_Lora_config));
  sllora.register_receive_handler(lora_on_receive);
  Serial.println(F("sllora.init done"));

  slbridge.bridge(&sllora);
}


int getBatInfo() {
#ifdef VBATPIN
    return int(analogRead(VBATPIN) * 6.45); // = *2*3.3/1024*1000
#else
    return 0.0;
#endif
}

//
// LOOP
//

void loop() {
  slbridge.update();
  //slesp.update();
}

// Handlers

void esp_on_receive(uint8_t *buf, uint8_t len) {
    Serial.print("slesp_on_receive: len: ");
    Serial.print(len);
    Serial.print(" msg: ");
    Serial.println((char*)buf);
}


void lora_on_receive(uint8_t *buf, uint8_t len) {
  Serial.print("sl_on_receive: len: ");
  Serial.print(len);
  Serial.print(" msg: ");
  Serial.println((char*)buf);
}

