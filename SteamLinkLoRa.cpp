#include <SteamLinkLoRa.h>

// #define LORA_SEND_BLUE_LED 2
//  #define LORA_RECEIVE_RED_LED 0

SteamLinkLora::SteamLinkLora(SL_NodeCfgStruct *config) : SteamLinkGeneric(config) {
  // initialize _config, _slid  and set _node_addr
  _config = _config;
  _slid = _config->slid;
  _node_addr = get_node_from_slid(_slid);
  if (_node_addr == get_node_from_slid(SL_DEFAULT_STORE_ADDR)) {
    FATAL("slid cannot be SL_DEFAULT_STORE_ADDR addr! ");
    while (1);
  }
}


bool SteamLinkLora::driver_can_send() {
  return true; // wait max 0 us
}

void SteamLinkLora::init(void *vconf, uint8_t config_length) {

  struct SteamLinkLoraConfig *_conf = (struct SteamLinkLoraConfig *) vconf;
  _encrypted = _conf->encrypted;
  _key = _conf->key;
  _mod_conf = _conf->mod_conf;

  if (config_length != sizeof(SteamLinkLoraConfig)) {
    FATAL("Received bad config struct, len should be: ");
    FATAL(sizeof(SteamLinkLoraConfig));
    FATAL(" is: ");
    FATALNL(config_length);
    while(1);
  }
  
  if (!_driver) {
/*
    INFONL("Resetting driver by toggling reset pin");
    pinMode(_reset_pin, OUTPUT);

    digitalWrite(_reset_pin, HIGH);
    delay(100);
    digitalWrite(_reset_pin, LOW);
    delay(50);
    digitalWrite(_reset_pin, HIGH);
    delay(100);
    digitalWrite(_reset_pin, LOW);
    delay(50);
    digitalWrite(_reset_pin, HIGH);
    delay(10);
*/  
    // TODO: radio params need to be extracted from config structure
    _driver = new LoRaClass();
    _driver->setPins(_cs_pin, _reset_pin, _interrupt_pin);// set CS, reset, IRQ pin
	  _driver->setFrequency(SL_LORA_DEFAULT_FREQUENCY * 1E6);
	  _driver->setSpreadingFactor(7);
	  _driver->enableCrc();
	  _driver->setSignalBandwidth(125E3);
	  _driver->setPreambleLength(8);
	  _driver->setCodingRate4(5);
	  _driver->enableCrc();

    if (!_driver->begin(SL_LORA_DEFAULT_FREQUENCY * 1E6)) {
		  FATAL("LoRa driver init failed");
		  while (true); 
    }
    INFO("LoRa Initialized\n");
  }

  
  INFONL("Setting Radio Parameters...");

  // Set frequency
//  _driver->setFrequency(SL_LORA_DEFAULT_FREQUENCY * 1E6);
//  INFO("Frequency set done\n");

  if (!update_modem_config()){
    FATAL("modemConfig failed");
    while (1);
  }
  INFO("Modem config done\n");
  randomSeed(analogRead(A0));
  //_driver->setCADTimeout(10000);
  // INFO("set CAD timeout\n");
//  _driver->setTxPower(SL_LORA_DEFAULT_TXPWR);

  INFO("set lora tx power\n");
  
  INFONL("LoRa driver ready to send!");
}

void SteamLinkLora::set_modem_config(uint8_t mod_conf) {
  _mod_conf = mod_conf;
}

bool SteamLinkLora::update_modem_config() {
  // Set modem configuration
  return true;
/*
  bool rc;
  if (_mod_conf > 3) {
    _driver->setModemRegisters(&modem_config[_mod_conf-4]);
    rc = true;
  }
  else
    rc = _driver->setModemConfig((RH_RF95::ModemConfigChoice) _mod_conf);
  if (!rc) {
    FATAL("setModemConfig failed with invalid config choice");
    while (1);
  }
  return rc;
*/
}

bool SteamLinkLora::driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid) {

  uint8_t to;
  int packetSize = _driver->parsePacket();

  if (packetSize == 0) 
    return false;
    
  INFO("SteamLinkLora::driver_receive len: ");
  INFO(packetSize);
  INFO("\n");

  int rcvlen = 0;
  while (_driver->available()) {
    driverbuffer[rcvlen] = (char)_driver->read();
    rcvlen+=1;
  }

  // determin slid for this packet
  if ((driverbuffer[0] & 0x01) == 0x01) { // IF IT'S A DATA PACKET
    struct data_header *header = (struct data_header *)&driverbuffer[0];
    slid = 0; // No "To" SLID ID
	  header->rssi = _driver->packetRssi();
  } else {
    struct control_header *header = (struct control_header *)&driverbuffer[0];
    slid = header->slid;
  }

  INFO("SteamLinkLora::driver_receive len: ");
  INFO(rcvlen);
  INFO(" to: ");
  INFO(slid);
  INFO(" packet: ");
  INFOPHEX(driverbuffer, rcvlen);
  _last_rssi = _driver->packetRssi();
  packet = (uint8_t *) malloc(rcvlen);
  INFO("SteamLinkLora::driver_receive: malloc "); Serial.println((unsigned int) packet, HEX);
  memcpy(packet,driverbuffer, rcvlen);
  packet_size = rcvlen;

/*
  if (to == get_node_from_slid(SL_DEFAULT_STORE_ADDR)) {
      slid = SL_DEFAULT_STORE_ADDR;
  } else if (to == get_node_from_slid(SL_DEFAULT_TEST_ADDR)) {
    slid = SL_DEFAULT_TEST_ADDR;
  } else {
    slid = (uint32_t) to | (get_mesh_from_slid(_slid) << 8);
  }
*/
  return true;
}


bool SteamLinkLora::driver_send(uint8_t* packet, uint8_t packet_size, uint32_t slid) {

#ifdef LORA_SEND_BLUE_LED
  pinMode(LORA_SEND_BLUE_LED, OUTPUT);
  digitalWrite(LORA_SEND_BLUE_LED, LOW);
#endif //LORA_SEND_BLUE_LED

  INFO("SteamLinkLora::driver_send len: ");
  INFO(packet_size);
  INFO(" to: ");
  INFO(slid);
  INFONL(" packet: ");
  _driver->beginPacket();
  _driver->write(packet, packet_size);
  _driver->endPacket();
  INFOPHEX(packet, packet_size);
  return true; 
}

// TODO: these are one-way functions
uint8_t SteamLinkLora::get_node_from_slid(uint32_t slid) {
  // use 0xFF mask to get the last 8 bits
  return (uint8_t) (slid & 0xFF);
}

// TODO: these are one-way functions
uint32_t SteamLinkLora::get_mesh_from_slid(uint32_t slid) {
  // drop the last 8 bits of slid
  return (uint32_t) (slid >> 8);
}

// pin defults re for Adafruit Feather M0 Lora
void SteamLinkLora::set_pins(uint8_t cs=8, uint8_t reset=4, uint8_t interrupt=3) {
  _cs_pin = cs;
  _reset_pin = reset;
  _interrupt_pin = interrupt;
}
