#include <SteamLinkGeneric.h>

#define RETRY_TIME_MS 10000

SteamLinkGeneric::SteamLinkGeneric(SL_NodeCfgStruct *config) : sendQ(SENDQSIZE) {
	_config = config;
	_slid = _config->slid;
	_encrypted = false;
	_key = NULL;
	_pkt_count_data = 1;		// don't use pkt_count 0
	_pkt_count_control = 1;		// don't use pkt_count 0
}

// TODO: rename to driver_init?
void SteamLinkGeneric::init(void *conf, uint8_t config_length) {
}

bool SteamLinkGeneric::send(uint8_t* buf) {
	return send_data(SL_OP_DS, buf, strlen((char*) buf) + 1);
}

void SteamLinkGeneric::update() {
	
	// Ensure that we are signed on
	if (!sign_on_complete) {
		sign_on_procedure();
		sign_on_complete = true;
	}

	// Handle receive on the PHY interface
	uint8_t* packet;
	uint8_t packet_length;
	uint32_t slid;
	bool received = driver_receive(packet, packet_length, slid);
	if (received) { // RECEIVED NEW PACKET ON THE PHY
		INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt");
		if (slid != SL_DEFAULT_TEST_ADDR) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is not test pkt");
			// Determine if we are data or control?
			if (is_data(packet[0])) { // DATA
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is a `data` pkt intended for store");
				if (_bridge_mode != unbridged) { // CAN BRIDGE
					INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Node is bridged. Handling packet.");
					handle_admin_packet(packet, packet_length);
				} else { // CANNOT BRIDGE
					WARN("SLID: "); WARN(_slid); WARNNL("SteamLinkGeneric:: update():: Node is unbridged. Dropping packet.");
					INFOPKT(packet, packet_length);
					INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
					free(packet); // Packet terminates. Must free.
				}
			} else { // CONTROL PACKETS
				INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is `control` pkt");
				// Determine if the packet is for me
				if (slid == _slid) { // MY PACKET
					INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is for this node");
					handle_admin_packet(packet, packet_length);
				} else { // NOT MY PACKET
					WARN("SLID: "); WARN(_slid); WARNNL("SteamLinkGeneric:: update():: Received control packet NOT addressed for this node.");
					WARNNL("WARNING: DROPPING PACKET!");
					INFOPKT(packet, packet_length);
					INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
					free(packet); // Packet terminates. Must free.
				}
			}
		} else { // Tell the store we saw a test packet
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received test pkt");
			send_tr(packet, packet_length);
		}
	}

	// Flush PHY queue
	if (driver_can_send() && sendQ.queuelevel()) {
		INFONL("SteamLinkGeneric::update: about to dequeue");
		packet = sendQ.dequeue(&packet_length, &slid);
		if (!driver_send(packet, packet_length, slid)) {
			WARNNL("SteamLinkGeneric::update driver_send dropping packet!!");
		}
		free(packet);
	}

	// Run Timer tasks
	// Timer task 1:
	// 		If we're waiting for ack and it's time to retry
	if (_waiting_for_ack && (millis() > (_last_retry_time + RETRY_TIME_MS)))  { 
		packet_length = _retry_packet_length;
		packet = (uint8_t*) malloc(packet_length); // should be free'd PHY side
		slid = _retry_slid;
		memcpy(packet, _retry_packet, packet_length);
		_last_retry_time = millis(); // update last send time
		generic_send(packet, packet_length,slid);
	}

	// Timer task 2: send a heartbeat if we have been silent for too long and we're not waiting for acks
	if (millis() > (_last_send_time + _config->max_silence*1000)) { // max_silence is in seconds
		INFONL("SteamLinkGeneric::update Sending heartbeat...");
		send_ss("OK");
	}
}

void SteamLinkGeneric::register_receive_handler(on_receive_handler_function on_receive) {
	_on_receive = on_receive;
}


void SteamLinkGeneric::register_bridge_handler(on_receive_bridge_handler_function on_receive) {
	_bridge_handler = on_receive;

}

bool SteamLinkGeneric::driver_send(uint8_t* packet, uint8_t packet_size, uint32_t slid) {
	return false;
}

bool SteamLinkGeneric::driver_can_send() {
	return true;
}

bool SteamLinkGeneric::send_enqueue(uint8_t* packet, uint8_t packet_length, uint32_t slid) {
	if (sendQ.enqueue(packet, packet_length, slid) == 0) {
		// TODO: Optional drop from the front of queue instead of back.
		WARNNL("SteamLinkGeneric::send_enqueue: WARN: sendQ FULL, pkt dropped");
	}
}

bool SteamLinkGeneric::driver_receive(uint8_t* &packet, uint8_t &packet_size, uint32_t &slid) {
	return false;
}

// returns true if send successful
bool SteamLinkGeneric::send_data(uint8_t op, uint8_t* payload, uint8_t payload_length) {
	if ( _waiting_for_ack && !is_transport(op))  { // Can only send TRANSPORT packets while waiting for ACK
		WARNNL("SteamlinkGeneric::send_data Warning: cannot send ADMIN and USER packets while waiting for ACK");
		return false;
	}
	uint8_t* packet;
	uint8_t packet_length = payload_length + sizeof(data_header);
	if (packet_length > SL_MAX_PACKET_LENGTH) {
		WARNNL("SteamlinkGeneric::send_data Warning: cannot send oversized data packet. Dropping");
		return false; // send failed.
	} else {
		packet = (uint8_t*) malloc(packet_length);
		INFO("SteamLinkGeneric::send_data malloc: "); Serial.println((unsigned int)packet, HEX);
		data_header *header = (data_header *)packet;
		header->op = op;
		header->slid = _slid;
		if (!is_transport(op)) {
			header->pkg_num = _pkt_count_data++;
			if (_pkt_count_data == 0) {
				_pkt_count_data++;
			}
		}
		header->rssi = _last_rssi;
		if (payload_length > 0) {
			memcpy(&packet[sizeof(data_header)], payload, payload_length);
		}
		// Cache packet in retry buffer if it needs to be ack'd
		if (needs_ack(op)) { // Packet should be acked
			_waiting_for_ack = true;
			memcpy(_retry_packet, packet, packet_length);
			_retry_packet_length = packet_length;
			_retry_slid = SL_DEFAULT_STORE_ADDR;
			_last_retry_time = millis();
		} 
		bool sent = generic_send(packet, packet_length, SL_DEFAULT_STORE_ADDR);

		if (is_transport(op)) {
			INFO("free transport payload: "); Serial.println((unsigned int)payload, HEX);
			free(payload);
		}

		return sent;
	}
}	

bool SteamLinkGeneric::send_td(uint8_t *td, uint8_t len) {
	uint8_t* packet = (uint8_t *)malloc(len);
	INFO("SteamLinkGeneric::send_td malloc: "); Serial.println((unsigned int)packet, HEX);
	memcpy(packet, td, len);
	uint8_t packet_length;
	INFONL("SteamLinkGeneric::send_td packet");
	bool sent = send_enqueue(packet, len, SL_DEFAULT_TEST_ADDR);  // update will free this
	return sent;
}

bool SteamLinkGeneric::send_on() {
	INFONL("SteamLinkGeneric::send_on packet: ");
	bool sent = send_data(SL_OP_ON, (uint8_t *)_config, sizeof(*_config));
	return sent;
}

bool SteamLinkGeneric::send_off(uint8_t seconds) {
	INFONL("SteamLinkGeneric::send_off packet: ");
	bool sent = send_data(SL_OP_OF, &seconds, sizeof(seconds));
	return sent;
}

bool SteamLinkGeneric::send_as(uint8_t ack_code) {
	INFONL("SteamLinkGeneric::send_as packet: ");
	bool sent = send_data(SL_OP_AS, &ack_code, sizeof(ack_code));
	return sent;
}

bool SteamLinkGeneric::send_ms(char* msg) {
	INFONL("SteamLinkGeneric::send_ms packet: ");
	bool sent = send_data(SL_OP_MS, (uint8_t *) msg, strlen(msg)+1);
	return sent;
}

bool SteamLinkGeneric::send_tr(uint8_t* payload, uint8_t payload_length) {
	INFONL("SteamLinkGeneric::send_tr packet: ");
	bool sent = send_data(SL_OP_TR, payload, payload_length);
	return sent;
}

bool SteamLinkGeneric::send_ss(char* status) {
	INFONL("SteamLinkGeneric::send_ss packet: ");
	bool sent = send_data(SL_OP_SS, (uint8_t *) status, strlen(status)+1);
	return sent;
}

void SteamLinkGeneric::handle_admin_packet(uint8_t* packet, uint8_t packet_length) {
	INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: handle_admin_packet():: Handling packet:");
	INFOPKT(packet, packet_length);
	uint8_t op = packet[0];
	if (op == SL_OP_DN) {          // CONTROL PACKETS
		INFONL("DN Packet Received");
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
		if (_on_receive != NULL) {
			_on_receive(packet, payload_length);
		}

	} else if (op == SL_OP_BN) {
		INFONL("BN Packet Received");
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
		uint32_t to_slid = ((control_header*) packet)->slid;
		generic_send(packet, payload_length, to_slid);

	} else if (op == SL_OP_GS) {
		INFONL("GetStatus Received");
		send_ss("OK");
		/* ---- TEST ONLY ----
		// This is useful to test whether the store is sending 
		// GS packets to the storeside driver

		if (_bridge_mode == storeside) {
			send_ss("OK");
		} else {
			INFONL("!!!! WARNING IGNORING NODESIDE GET STATUS !!!!");
		}
		// ------------------- */
	} else if (op == SL_OP_TD) {
		INFONL("Transmit Test Received");
		send_as(SL_ACK_SUCCESS);
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
		send_td(packet, payload_length);

	} else if (op == SL_OP_SC) {
		INFONL("Set Config Received");
		if (_waiting_for_ack) {
			_waiting_for_ack = false;
			uint8_t* payload = packet + sizeof(control_header);
			uint8_t payload_length = packet_length - sizeof(control_header);
			memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
			short vers = ((SL_NodeCfgStruct*) packet)->version;
			if (vers == NODE_CONFIG_VERSION) {
				if ( payload_length != sizeof(SL_NodeCfgStruct)) {
    				FATAL("FATAL: Received bad generic config struct. Should be:");
					FATAL(sizeof(SL_NodeCfgStruct));
    				FATAL(", is: ");
    				FATALNL(payload_length);
					send_as(SL_ACK_SIZE_ERR);
  				} else {
					// TODO: keep backup of config pointer in non volatile memory?
					_config = (SL_NodeCfgStruct*) malloc(sizeof(SL_NodeCfgStruct));
					memcpy(_config, packet, payload_length);
					_slid = _config->slid;
					
					// TODO : Pass driver initialization config to init function
					// TODO : INFONL("Passing payload as config to init");
					// TODO : init(packet, payload_length);
					send_as(SL_ACK_SUCCESS);
				}
			} else {
				WARN("Warning: received packet with unknown version: "); WARNNL(vers);
				send_as(SL_ACK_VERSION_ERR);
			}
		} else {
			WARNNL("Warning: Unexpected Set Config. Did node send an ON msg?");
			send_as(SL_ACK_UNEXPECTED);
		}
		INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
		free(packet);
	} else if (op == SL_OP_BC) {
		INFONL("BootCold Received");
		INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
		free(packet);
		while(1);    // watchdog will reset us

	} else if (op == SL_OP_BR) {
		INFONL("BootReset Received");
		// TODO: actually reset the radio
		INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
		free(packet);
	} else if (op == SL_OP_AN) {
		INFONL("AN Received");
		if (!_waiting_for_ack) {
			WARNNL("Warning: Unexpected AN received");
		}
		INFO("free packet: "); Serial.println((unsigned int)packet, HEX);
		free(packet);
		_waiting_for_ack = false;
	}
	else if ((op & 0x1) == 1) {     // we've received a DATA PACKET
		send_data(SL_OP_BS, packet, packet_length); 
	}
}

void SteamLinkGeneric::sign_on_procedure() {
	send_on();
}

void SteamLinkGeneric::set_bridge(BridgeMode mode) {
	_bridge_mode = mode;
}

uint32_t SteamLinkGeneric::get_slid() {
	return _slid;
}

bool SteamLinkGeneric::generic_send(uint8_t* packet, uint8_t packet_length, uint32_t slid) {
	INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: send pkt via BRIDGE or PHY:");	
	INFOPKT(packet, packet_length);

	bool rc = true;

	if ( is_data(packet[0]) ) { // DATA
		if  (_bridge_mode == storeside ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via PHY");
			rc = send_enqueue(packet, packet_length, slid);
		} else if ( _bridge_mode == nodeside  ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via BRIDGE");
			_bridge_handler(packet, packet_length, slid);
		} else if ( _bridge_mode == unbridged ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via PHY");
			rc = send_enqueue(packet, packet_length, slid);
		}
	} else { // CONTROL
		if ( _bridge_mode == nodeside  ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via PHY");
			rc = send_enqueue(packet, packet_length, slid);
		} else if ( _bridge_mode == unbridged ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via PHY");
			WARNNL("WARNING: Sending a control packet as an unbridged node");
			rc = send_enqueue(packet, packet_length, slid); // TODO: is this even a valid case?
		} else  if ( _bridge_mode == storeside ) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: generic_send():: sending pkt via BRIDGE");
			_bridge_handler(packet, packet_length, slid);
		}
	}
	
	_last_send_time = millis();

	if (!rc) {
		WARNNL("SteamLinkGeneric::generic_send: send failed!!");
	}
	return rc;
}

bool SteamLinkGeneric::is_transport(uint8_t op) {
	return (
		( op == SL_OP_BS) ||
		( op == SL_OP_TR)
	);
}

bool SteamLinkGeneric::needs_ack(uint8_t op) {
	return (
		( op == SL_OP_DS) ||
		( op == SL_OP_ON)
	);
}

bool SteamLinkGeneric::is_data(uint8_t op) {
		return ((op & 0x1) == 1); 
}