# SteamLink

This document includes a high-level summary of project goals, protocol, and implementation.

## Introduction

SteamLink is a low-power netwoking platform. The SteamLink vision is to give makers the ability to rapidly and easily deploy fully self-hosted, robust, and secure networks of low-powered devices. Potential applications include data-driven citizen science projects and automation systems.

Currently the SteamLink project plans to support LoRa and WiFi devices.

**WARNING:** SteamLink is in active development and the protocol is subject to change. The current codebase has limited security features implemented and some portions are untested. Use at your own risk.

### Ecosystem Layout

```
                                                      +-----------+
                                        +-----------> | WiFi Node |
            +----------+                |             +-----------+
     +----> | Database |                |
     |      +----------+                |
     v                                  v
+---------------+              +-------- ---------+
|               |              |                  |
|               |              |                  |       +-------+-------+       +-----------+
|               |              |                  | <---> |  WiFi | LoRa  | <---> | LoRa Node |
|     Store     | <----------> |    MQTT Broker   |       +-------+-------+       +-----------+
|               |              |                  |             BRIDGE
|               |              |                  |
|               |              |                  |
+---------------+              +------------------+
      ^
      |         +-----------+
      |         |  Web      |
      +-------> |  Console  |
                +-----------+
```
_Made using [this ascii editor](http://asciiflow.com/)_

### Hardware Supported

The SteamLink client arduino library have currently been tested on:

1. Adafruit LoRa featherboards
2. ESP8266
3. ESP32

The store is implemented in Python and most unix like systems should be able to support it.

## Components of the SteamLink Network

A SteamLink network comprises:

### 1. SteamLink Nodes

SteamLink Nodes can be thought of as "clients" in a SteamLink network. There are two types of nodes:
- Standard nodes: These nodes have a single physical interface to send and receive packets. As of the current SteamLink version, traffic from the store terminates at a standard node.
- Bridge node: These nodes have two physical interfaces and can pass packets from one physical interface to another. 

### 2. SteamLink Store

The SteamLink store can be thought of as the "backend" of a SteamLink network. The SteamLink store provides the database, web-console, and a default message brokering facility for the network.

**NB:** All nodes in a SteamLink network have a unique SteamLink ID or `slid` for short. Bridge nodes have two `slid`  

## Packet Flow

The SteamLink packet-flow is in two directions: "node -> store" and "store -> node". All packets include a header followed by a payload.

### 1. `data`: node -> store 

`data` packets as they usually contain sensor data and status messages.

#### Data MQTT Channel
Any node that can communicate directly to MQTT can send its data messages to: `SteamLink/<slid>/data`

#### Data Header format

| Field         | Bits   |  Description                                                |
| ------------- |-------:| :----------------------------------------------------------:|
| `op`          |       8| Data packet op code (see below)                             |
| `slid`        |      32| SteamLink ID of the node creating the packet                |
| `pkg_num`     |      16| Packet number/count set by the node creating the packet     |
| `rssi`        |       8| Describes the signal strength of the previous hop           |


Data packets look like:
```
+----+------+---------+------+-----------+
| op | slid | pkg_num | rssi | payload...|
+----+------+---------+------+-----------+
```
#### Data Packet OP codes:

**NB** Data message op codes have an odd bottom bit

| OP code       | Hex    | Type   | Resp. from store | Payload            | Description                    |
| ------------- |-------:|:------:| -----------------|--------------------|:------------------------------:|
| `DS`          |    0x31| USER   | `AN`             | User payload       | User packet to store           |
| `BS`          |    0x33| ADMIN  | none             | Encapsulated packet| Bridge data to store           |
| `ON`          |    0x35| ADMIN  | `SC`             | Node config struct | Sign on packet                 |
| `AS`          |    0x37| ADMIN  | none             | 8 bit code         | Ack from node -> store         |
| `MS`          |    0x39| USER   | none             | string             | Logging and messaging service  |
| `TR`          |    0x3B| ADMIN  | none             | Received test data | Test packet seen               |
| `SS`          |    0x3D| SHARED | none             | string             | Status service                 |
| `OF`          |    0x3F| ADMIN  | none             | int time in secs   | Sign-off, do not disturb       |

### 2. `control`: store -> node

`control` packets usually contain messages to configure and command the nodes.

#### Control MQTT Channel
Any node that can communicate directly to MQTT receives its control messages on: `SteamLink/<slid>/control`

#### Control Header format

| Field         | Bits   |  Description                                                |
| ------------- |-------:| :----------------------------------------------------------:|
| `op`          |       8| Control packet op code (see below)                          |
| `slid`        |      32| SteamLink ID of the destination node of the packet          |
| `pkg_num`     |      16| Packet number/count set by the node creating the packet     |

Control packets look like:
```
+----+------+---------+-----------+
| op | slid | pkg_num | payload...|
+----+------+---------+-----------+
```
#### Control Packet OP codes:

**NB** Control message op codes have an even bottom bit

| OP code       | Hex    | Type   | Resp. from node  | Payload            | Description                    |
| ------------- |-------:|:------:| -----------------|--------------------|:------------------------------:|
| `DN`          |    0x30| USER   | `AS`             | User payload       | User packet from store         |
| `BN`          |    0x32| ADMIN  | none             | Encapsulated packet| Bridge data from store         |
| `GS`          |    0x34| ADMIN  | `SS`             | none               | Get status from node           |
| `TD`          |    0x36| ADMIN  | none             | Test data to send  | Transmit test packet via radio |
| `SC`          |    0x38| ADMIN  | `AS`             | Node config struct | Set new configuration          |
| `BC`          |    0x3A| ADMIN  | none             | none               | Reboot node                    |
| `BR`          |    0x3C| ADMIN  | `AS`             | Radio params       | Radio reset `[TBD]`            |
| `AN`          |    0x3E| ADMIN  | none             | 8 bit code         | Ack from store -> node         |


### Acknowledgement Packet Codes

| Code      | `AN`                      | `AS`                      |
|-----------|---------------------------|---------------------------|
| 0x00      | Success                   | Success                   |
| 0x01      | Supressed duplicate pkt   | Supressed duplicate pkt   |
| 0x02      | Unexpected pkt, dropping  | Unexpected pkt, dropping  |

### Node Configuration

The steamlink node structure looks like this:

```
struct SL_NodeCfgStruct {
    uint8_t  version;
    uint32_t slid;
    char name[10];
    char description[32];
    float gps_lat;
    float gps_lon;
    short altitude;
    uint8_t max_silence; // in seconds
    bool battery_powered;
    uint8_t radio_params; // radio params need to be interpreted by drivers
};
```
