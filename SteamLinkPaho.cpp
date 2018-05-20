#ifdef UNIX

#include <SteamLinkPaho.h>
#include <SL_RingBuff.h>
#include <string.h>


#define MQTTQSIZE 10

SL_RingBuff mqttQ(MQTTQSIZE);

SteamLinkPaho::SteamLinkPaho(SL_NodeCfgStruct *config) : SteamLinkGeneric(config) {
	_config = _config;
	_slid = _config->slid;
	create_pub_str(_pub_str, _slid);
	create_sub_str(_sub_str, _slid);
}

void SteamLinkPaho::init(void *vconf, uint8_t config_length) {
	INFO("Initializing SteamLinkPaho\n");
	if ( config_length != sizeof(SteamLinkPahoConfig)) {
    	FATAL("Received bad config struct. Should be:");
		FATAL(sizeof(SteamLinkPahoConfig));
    	FATAL(", is: ");
    	FATALNL(config_length);
    	while(1);
  	}
  	_conf = (struct SteamLinkPahoConfig *) vconf;

    _ipstack = new IPStack();
    _client = new MQTT::Client<IPStack, Countdown>(*_ipstack);
    
    INFONL("Beginning TCP connection...");
    int rc = _ipstack->connect(_conf->sl_server, _conf->sl_serverport);
    if (rc != 0)
	    printf("rc from TCP connect is %d\n", rc);

    _data.MQTTVersion = 3;
    _data.clientID.cstring = (char*) _conf->sl_conid;

    INFONL("MQTT connecting");
    rc = _client->connect(_data);
	if (rc != 0)
	    printf("rc from MQTT connect is %d\n", rc);
	printf("MQTT connected\n");
    
    rc = _client->subscribe(_sub_str, MQTT::QOS0, _sub_callback);   
    if (rc != 0)
        printf("rc from MQTT subscribe is %d\n", rc);

}

bool SteamLinkPaho::driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid) {
	// first make sure we're still connected

    _client->yield(100);

	if (mqttQ.queuelevel()) {
    	_last_rssi = 255;
   
    	packet = mqttQ.dequeue(&packet_size, NULL);

    	INFO("SteamLinkPaho::driver_receive len: ");
    	INFO(packet_size);
    	INFO(" packet: ");
   		INFOPHEX(packet, packet_size);
   		// TODO: see comment above re: MQTT nodes can only speak to store for now

  		if ((packet[0] & 0x01) == 0x01) { // IF IT'S A DATA PACKET
    		struct data_header *header = (struct data_header *)&packet[0];
    		slid = 0; // No "To" SLID ID
  		} else {
    		struct control_header *header = (struct control_header *)&packet[0];
    		slid = header->slid;
  		}
  		return true;
  
  	} else {
    	return false;
  	}
}

bool SteamLinkPaho::driver_send(uint8_t* packet, uint8_t packet_size, uint32_t slid) {
	INFO("Sending user string over mqtt\n");
	// TODO: for now, MQTT nodes can only send to one SLID, ie store_slid
	// this can be exanded in the future. We ignore the input SLID

	// TODO also MQTT nodes cannot send MQTT test packages for now

    MQTT::Message message;

    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) packet;
    message.payloadlen = packet_size;
    int rc = _client->publish(_pub_str, message);
	if (rc != 0)
		printf("Error %d from sending QoS 0 message\n", rc);
    return true;
}


void SteamLinkPaho::create_pub_str(char* topic, uint32_t slid) {
	snprintf(topic, SL_PAHO_DEFAULT_TOPIC_LEN, "SteamLink/%u/data", slid);
}

void SteamLinkPaho::create_sub_str(char* topic, uint32_t slid) {
	snprintf(topic, SL_PAHO_DEFAULT_TOPIC_LEN, "SteamLink/%u/control", slid);
}

void SteamLinkPaho::_sub_callback(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;

  	char *msg;  
  	// TODO: check max len
  	msg = (char *) malloc((int)message.payloadlen);
  	INFO("SteamLinkPaho::_sub_callback malloc: "); INFOHEX(msg); INFONL(" ");

  	memcpy(msg, message.payload, (int) message.payloadlen);
  	if (mqttQ.enqueue((uint8_t *)msg, (int) message.payloadlen, NULL) == 0) {
    	WARNNL("SteamLinkPaho::_sub_callback: WARN: mqttQ FULL, pkt dropped");
  	}
}

#endif // ifdef UNIX