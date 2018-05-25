#ifndef STEAMLINKLORA_H
#define STEAMLINKLORA_H

#include <LoRa.h>
#include <SteamLinkGeneric.h>
#include <SteamLink.h>
#include <SL_RingBuff.h>

#define SL_DEFAULT_SLID_SIZE 32 // in bits
#define SL_LORA_DEFAULT_NODE_SIZE 8 // in bits
#define SL_LORA_DEFAULT_MESH_SIZE  SL_DEFAULT_SLID_SIZE - SL_LORA_DEFAULT_NODE_SIZE

#define SL_LORA_DEFAULT_BRIDGE_ADDR 1

#define SL_LORA_MAX_MESSAGE_LEN 255

#define SL_LORA_DEFAULT_TXPWR 23
#define SL_LORA_DEFAULT_FREQUENCY 915

#define SL_LORA_DEFAULT_FLAGS 0
#define SL_LORA_TEST_FLAGS 1

//#define LORASENDQSIZE 10
/*
This library needs the following defines:

In the SteamLink LoRa driver the SLID, node_addr and mesh_addr are related:
slid is 32 bits
MSB...........................LSB
[24 bit mesh_id][8 bit node_addr]
*/


#pragma pack(push,1)
struct SteamLinkLoRaConfig {
  bool encrypted;
  uint8_t *key;
  uint8_t mod_conf;
};
#pragma pack(pop)

class SteamLinkLoRa : public SteamLinkGeneric {

 public:

  // constructor
//  SteamLinkLoRa(uint32_t slid);
  SteamLinkLoRa(SL_NodeCfgStruct *config);

  virtual void init(void *conf, uint8_t config_length);

  virtual bool driver_send(uint8_t* packet, uint8_t packet_length, uint32_t slid);

  virtual bool driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid);
  /// get_addrs_from_slid
  /// \brief LoRa driver uses the 32 bit slid
  ///  24 bits are mesh_id and 8 bits are node_addr
  ///  this function is to convert from slid to node_addr and mesh_id
  /// \param slid
  /// \returns node_addr
  uint8_t get_node_from_slid(uint32_t slid);

  uint32_t get_mesh_from_slid(uint32_t slid);

  void set_pins(uint8_t cs, uint8_t reset, uint8_t interrupt);

  bool driver_can_send();

 private:

  // config info
  struct SteamLinkLoRaConfig *_conf;
  uint8_t *_key = NULL;
  uint8_t _encrypted = false;
  
  LoRaClass *_driver = NULL;

  bool update_modem_config();

  void set_modem_config(uint8_t mod_conf);

  // only the driver needs to be aware of node_addr
  uint8_t _node_addr;

  // LORA specific pins
  uint8_t _cs_pin;
  uint8_t _reset_pin;
  uint8_t _interrupt_pin;
  int _implicit_mode = 1;

  // modem_config
  uint8_t _mod_conf = 0;

  //  SL_RingBuff loraSendQ(LORASENDQSIZE);

  // LORA driver stuff
  uint8_t driverbuffer[SL_LORA_MAX_MESSAGE_LEN];

  // Custom modem config
  // TODO: for LoRa driver
/*
  RH_RF95::ModemConfig modem_config[1] = {
    { // conf number 4, see https://lowpowerlab.com/forum/general-topics/issue-with-lora-long-range-modes-and-rfm95rfm96/
      .reg_1d = 0x78, // Reg 0x1D: BW=125kHz, Coding=4/8, Header=explicit
      .reg_1e = 0xc4, // Reg 0x1E: Spread=4096chips/symbol, CRC=enable
      .reg_26 = 0x0c  // Reg 0x26: LowDataRate=On, Agc=On
    }
  };
*/
};
#endif
