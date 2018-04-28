#ifndef STEAMLINK_H
#define STEAMLINK_H

#include <Arduino.h>

#define NODE_CONFIG_VERSION 1

//   Node configuration data, as stored in flash
#pragma pack(push,1)
struct SL_NodeCfgStruct {
    uint8_t  version;
    uint32_t slid;
    char name[10];
    char description[32];
    float gps_lat;
    float gps_lon;
    short altitude;
    uint8_t max_silence; // in ms
    bool battery_powered;
    uint8_t radio_params; // radio params need to be interpreted by drivers
};
#pragma pack(pop)

typedef void (*on_receive_handler_function)(uint8_t* buffer, uint8_t size); // user
typedef void (*on_receive_bridge_handler_function)(uint8_t* packet, uint8_t packet_length, uint32_t to_slid); // admin

#define  MIN(a,b) (((a)<(b))?(a):(b))
#define  MAX(a,b) (((a)>(b))?(a):(b))

// admin_control message types: EVEN, 0 bottom bit
#define SL_OP_DN 0x30  // data to node, repy with AS if qos
#define SL_OP_BN 0x32  // BN
#define SL_OP_GS 0x34  // get status, reply with SS message
#define SL_OP_TD 0x36  // transmit a test message via radio
#define SL_OP_SC 0x38  // set radio paramter to x
#define SL_OP_BC 0x3A  // restart node
#define SL_OP_BR 0x3C  // reset the radio
#define SL_OP_AN 0x3E  // store -> node acknowledgment

// admin_data message types: ODD, 1 bottom bit
#define SL_OP_DS 0x31  // data to store
#define SL_OP_BS 0x33  // bridge to store
#define SL_OP_ON 0x35  // send status on to store
#define SL_OP_AS 0x37  // acknowledge from node -> store
#define SL_OP_MS 0x39  // log user message to store
#define SL_OP_TR 0x3B  // Received Test Data
#define SL_OP_SS 0x3D  // status info and counters

// DEBUG INSTRUCTIONS:
//     Change below line or comment out to change the level of debugging
//     Make sure Serial is defined if DEBUG is enabled
//     Debugger uses Serial.println() to debug
#define DEBUG_ENABLED DEBUG_LEVEL_INFO

#define DEBUG_PACKET_VERBOSE 0

#define DEBUG_LEVEL_INFO   4
#define DEBUG_LEVEL_WARN   3
#define DEBUG_LEVEL_ERR    2
#define DEBUG_LEVEL_FATAL  1
#define DEBUG_LEVEL_NONE   0

#if DEBUG_ENABLED >= DEBUG_LEVEL_INFO
 #define INFONL(text) Serial.println(text)
 #define INFO(text) Serial.print(text)
 #define INFOPHEX(data, len) phex(data, len)
 #define INFOPKT(packet, packet_length) print_packet(packet, packet_length)
#else 
 #define INFONL(text) ((void)0)
 #define INFO(text) ((void)0)
 #define INFOPHEX(data, len) ((void)0)
 #define INFOPKT(packet, packet_length) ((void)0)
#endif

#if DEBUG_ENABLED >= DEBUG_LEVEL_WARN
 #define WARNNL(text) Serial.println(text)
 #define WARN(text) Serial.print(text)
 #define WARNPHEX(data, len) phex(data, len)
#else 
 #define WARNNL(text) ((void)0)
 #define WARN(text) ((void)0)
 #define WARNPHEX(data, len) ((void)0)
#endif

#if DEBUG_ENABLED >= DEBUG_LEVEL_ERR
 #define ERRNL(text) Serial.println(text)
 #define ERR(text) Serial.print(text)
 #define ERRPHEX(data, len) phex(data, len)
#else 
 #define ERRNL(text) ((void)0)
 #define ERR(text) ((void)0)
 #define ERRPHEX(data, len) ((void)0)
#endif

#if DEBUG_ENABLED >= DEBUG_LEVEL_FATAL
 #define FATALNL(text) Serial.println(text)
 #define FATAL(text) Serial.print(text)
 #define FATALPHEX(data, len) phex(data, len)
#else 
 #define FATALNL(text) ((void)0)
 #define FATAL(text) ((void)0)
 #define FATALPHEX(data, len) ((void)0)
#endif

////////////////////////////////////////
// HEADER STRUCTURES
////////////////////////////////////////

/// DATA PACKETS ///
#pragma pack(push,1)
struct data_header {
  uint8_t op;
  uint32_t slid;
  uint16_t pkg_num;
  uint8_t rssi;
};
#pragma pack(pop)

#pragma pack(push,1)
struct control_header {
  uint8_t op;
  uint32_t slid;
  uint16_t pkg_num;
};
#pragma pack(pop)

void phex(uint8_t *data, unsigned int length);

void print_packet(uint8_t* packet, uint8_t packet_length);

void print_op_code(uint8_t op);

enum BridgeMode { unbridged, storeside, nodeside };

#endif
