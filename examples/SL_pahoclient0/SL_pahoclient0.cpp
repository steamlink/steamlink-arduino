// SL_pahoclient0.cpp
// -*- mode: C++ -*-
// https://steamlink.net/

#define VER "1"

#include <SteamLink.h>
#include <SteamLinkPaho.h>
#include "SL_Credentials.h"
#include <ctime>

#define SL_ID_PAHO 0x158

struct SL_NodeCfgStruct Pahoconfig = {
    1,                  // version
	SL_ID_PAHO,              // slid
	"PahoMac",          // name
	"Running on Mac",     // description
	43.43,              // gps_lat
	-79.23,             // gps_lon
	180,                // altitude
	45,                 // max_silence in s
    false,              // battery powered
    1                   // radio_params
};

void paho_on_receive(uint8_t* buffer, uint8_t size);

SteamLinkPaho slpaho(&Pahoconfig);

/* Packet building */
uint8_t data[100];
int packet_num = 0;

double send_every_secs = 0.01; // 10 seconds

int main()
{   
    slpaho.init((void *) &sl_paho_config, sizeof(sl_paho_config));
    slpaho.register_receive_handler(paho_on_receive);
	
	std::clock_t lastsent;
	std::clock_t now;
	lastsent = std::clock();
	double duration;

    while(1) {
	now = std::clock();
	duration = ((now - lastsent) / (double) CLOCKS_PER_SEC);
	if (duration > send_every_secs) {
		lastsent = now;
		slpaho.send((uint8_t*) "Hello World!");
	}
        slpaho.update();
    }
    return 0;
}

void paho_on_receive(uint8_t *buf, uint8_t len) {
    INFONL("received");
}
