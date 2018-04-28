#ifndef STEAMLINKBRIDGE_H
#define STEAMLINKBRIDGE_H

#include <SteamLinkGeneric.h>
#include <SteamLink.h>

class SteamLinkBridge {

public:

  SteamLinkBridge(SteamLinkGeneric *storeSideDriver);

  void bridge(SteamLinkGeneric *nodeSideDriver);

  void update();

private:

  void init();

  static SteamLinkGeneric *_storeSideDriver;

  //TODO make list
  static SteamLinkGeneric *_nodeSideDriver;
  static bool _init_done;
  static void router(uint8_t* packet, uint8_t packet_length, uint32_t slid);

};

#endif
