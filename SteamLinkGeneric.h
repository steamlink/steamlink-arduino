#ifndef STEAMLINKGENERIC_H
#define STEAMLINKGENERIC_H

#include <SteamLink.h>
#include <SL_RingBuff.h>

#define SL_DEFAULT_STORE_ADDR 0x1
#define SL_DEFAULT_TEST_ADDR  0xFFFFFFFF

#define SL_MAX_PACKET_LENGTH 255

#define SENDQSIZE 10

class SteamLinkGeneric {

    public:

        // constructor
        SteamLinkGeneric(SL_NodeCfgStruct *config);

        virtual void init(void *conf, uint8_t config_length);

        /// \send
        /// \brief for user to send string to store
        /// \param buf a nul terminated string to send
        /// \returns true if message sends succesfully
        virtual bool send(uint8_t* buf);

        /// ADMIN DATA PACKETS

        virtual bool send_data(uint8_t op,uint8_t* payload, uint8_t payload_length);
  
        virtual bool send_td(uint8_t *td, uint8_t len);

        virtual bool send_on();
        
        virtual bool send_off(uint8_t seconds);

        virtual bool send_as(uint8_t ack_code);

        virtual bool send_ms(char *msg);

        virtual bool send_tr(uint8_t* payload, uint8_t payload_length);

        virtual bool send_ss(char *status);

        virtual void update();

        virtual void handle_admin_packet(uint8_t* packet, uint8_t packet_length);

        /// HANDLER REGISTRATIONS

        virtual void register_receive_handler(on_receive_handler_function on_receive);
        
        /// \register_admin_handler
        /// \brief if the message is not for this node AND there is a bridge registered, send to bridge handler
        virtual void register_bridge_handler(on_receive_bridge_handler_function on_receive);

        virtual void sign_on_procedure();

        virtual bool send_enqueue(uint8_t* packet, uint8_t packet_length, uint32_t slid);
        /// DRIVER LEVEL CALLS

        /// driver_send
        /// \brief sends packet to the slid
        /// \param packet is a pointer for the packet to send
        /// \param packet_size is the size of the packet
        /// \param slid is the steamlink id of the receiver node
        virtual bool driver_send(uint8_t* packet, uint8_t packet_length, uint32_t slid);

        virtual bool driver_can_send();

        virtual bool driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid);

        virtual uint32_t get_slid();

        virtual void set_bridge(BridgeMode mode);

        virtual bool generic_send(uint8_t* packet, uint8_t packet_length, uint32_t slid);


    protected:

        bool is_transport(uint8_t op);
        bool is_data(uint8_t op);
        bool needs_ack(uint8_t op);
   
        struct SL_NodeCfgStruct *_config;
        uint32_t _slid;

        // handlers
        on_receive_handler_function _on_receive = NULL;
        on_receive_bridge_handler_function _bridge_handler = NULL;

        // encryption mode
        bool _encrypted;
        uint8_t* _key;

        // packet counters
        uint16_t  _pkt_count_data, _pkt_count_control;

        uint8_t _last_rssi = 0;

        // bridge mode
        // _bridge_mode
        // 0 not connected to bridge
        // 1 is store_side
        // 2 is node_side
        BridgeMode _bridge_mode = unbridged;

        bool sign_on_complete = false;
        uint32_t  _last_send_time = 0;

        // Retry buffer
        uint8_t  _retry_packet[SL_MAX_PACKET_LENGTH];
        uint8_t  _retry_packet_length;
        uint32_t _retry_slid;
        uint32_t _last_retry_time = 0;
        
        bool _waiting_for_ack = false;

        SL_RingBuff sendQ;

    private:

};
#endif
