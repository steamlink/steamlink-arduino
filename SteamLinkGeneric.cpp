#include <SteamLinkGeneric.h>

#define RETRY_TIME_DIVIDER 4

SteamLinkGeneric::SteamLinkGeneric(SL_NodeCfgStruct *config) : sendQ(SENDQSIZE) {
	_config = config;
	_slid = _config->slid;
	_encrypted = false;
	_key = NULL;
	_pkt_count_data = 1;		// don't use pkt_count 0
	_pkt_count_control = 1;		// don't use pkt_count 0
}

void SteamLinkGeneric::init(void *conf, uint8_t config_length) {
}

bool SteamLinkGeneric::send(uint8_t* buf) {
	return send_data(SL_OP_DS, buf, strlen((char*) buf) + 1);
}

void SteamLinkGeneric::update() {
	
	// Sign on
	if (!sign_on_complete) {
		sign_on_procedure();
		sign_on_complete = true;
		last_send_time = millis();
	}
	uint8_t* packet;
	uint8_t packet_length;
	uint32_t slid;
	bool received = driver_receive(packet, packet_length, slid);
	
	if (received) { // RECEIVED NEW PACKET ON THE PHY
		INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt");
		if (slid != SL_DEFAULT_TEST_ADDR) {
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is not test pkt");
			// Determine if we are data or control?
			if ((packet[0] & 0x01) == 0x01) { // DATA
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received pkt is a `data` pkt intended for store");
				if (_bridge_mode != unbridged) { // CAN BRIDGE
					INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Node is bridged. Handling packet.");
					handle_admin_packet(packet, packet_length);
				} else { // CANNOT BRIDGE
					WARN("SLID: "); WARN(_slid); WARNNL("SteamLinkGeneric:: update():: Node is unbridged. Dropping packet.");
					INFOPKT(packet, packet_length);
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
				}
			}
		} else { // Tell the store we saw a test packet
			INFO("SLID: "); INFO(_slid); INFONL("SteamLinkGeneric:: update():: Received test pkt");
			send_tr(packet, packet_length);
		}
	}
		
	if (!_retry_buffer_full && sendQ.queuelevel()) { // IF empty retry buffer and nonzero queue
		INFONL("SteamLinkGeneric::update: about to dequeue");
		_retry_packet = sendQ.dequeue(&_retry_packet_length, &_retry_slid);

		if (_retry_slid != SL_DEFAULT_TEST_ADDR) { // IF NOT TEST PACKET
			if ((_retry_packet[0] & 0x1) == 1) { // IF DATA
				((data_header*) _retry_packet)->pkg_num = _pkt_count_data++;
				if (_pkt_count_data == 0) {
					_pkt_count_data++;
				}
				uint8_t op = ((data_header*) _retry_packet)->op;
				if ( op == SL_OP_DS || op == SL_OP_ON ) { // we expect AN for these op's
					_waiting_for_ack = true;
				}
			} else { // IF CONTROL
				((control_header*) _retry_packet)->pkg_num = _pkt_count_control++;
				if (_pkt_count_control == 0) {
					_pkt_count_control++;
				}
			} // IF CONTROL
		} // IF NOT TEST

		// We have gauranteed that the _retry packet is full
		if (!driver_send(_retry_packet, _retry_packet_length, _retry_slid)) {
			WARNNL("SteamLinkGeneric::update driver_send dropping packet!!");
		}			
		
		if (!_waiting_for_ack) {
			free(_retry_packet); // packets should be free after ack if waiting
		} else {
			_retry_buffer_full = true;
		}
	} // IF empty retry buffer and nonzero queue

	// TIMERS
	if ((millis() > (last_send_time + _config->max_silence*1000/RETRY_TIME_DIVIDER)) && _waiting_for_ack )  { // max_silence is in seconds))
		if (!driver_send(_retry_packet, _retry_packet_length, _retry_slid)) {
			WARNNL("SteamLinkGeneric::update driver_send dropping packet!!");
		}			
		
	}
	// send a heartbeat if we have been silent for too long
	if (millis() > (last_send_time + _config->max_silence*1000)) { // max_silence is in seconds
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

bool SteamLinkGeneric::send_data(uint8_t op, uint8_t* payload, uint8_t payload_length) {
	uint8_t* packet;
	uint8_t packet_length = payload_length + sizeof(data_header);
	packet = (uint8_t*) malloc(packet_length);
	INFO("SteamLinkGeneric::send_data malloc: "); Serial.println((unsigned int)packet, HEX);
	data_header *header = (data_header *)packet;
	header->op = op;
	header->slid = _slid;
	header->pkg_num = 0; // this will be filled in right before we send
	header->rssi = _last_rssi;
	if (payload_length > 0) {
		memcpy(&packet[sizeof(data_header)], payload, payload_length);
	}
	bool sent = generic_send(packet, packet_length, SL_DEFAULT_STORE_ADDR);
	return sent;
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

bool SteamLinkGeneric::send_as() {
	INFONL("SteamLinkGeneric::send_as packet: ");
	bool sent = send_data(SL_OP_AS, NULL, 0);
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
		send_as();
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
		send_td(packet, payload_length);

	} else if (op == SL_OP_SC) {
		INFONL("Set Config Received");
		if (_waiting_for_ack) {
			_waiting_for_ack = false;
			_retry_buffer_full = false;
			free(_retry_packet);
			uint8_t* payload = packet + sizeof(control_header);
			uint8_t payload_length = packet_length - sizeof(control_header);
			memcpy(packet, payload, payload_length);    // move payload to start of allc'd pkt
			short vers = ((SL_NodeCfgStruct*) packet)->version;
			if (vers == NODE_CONFIG_VERSION) {
				INFONL("Passing payload as config to init");
				init(packet, payload_length);
			} else {
				WARN("Warning: received packet with unknown version: "); WARNNL(vers);
			}
		} else {
			WARNNL("Warning: Unexpected Set Config. Did node send an ON msg?");
		}
	} else if (op == SL_OP_BC) {
		INFONL("BootCold Received");
		while(1);    // watchdog will reset us

	} else if (op == SL_OP_BR) {
		INFONL("BootReset Received");
		// TODO: actually reset the radio
	} else if (op == SL_OP_AN) {
		INFONL("AN Received");
		if (!_waiting_for_ack) {
			WARNNL("Warning: Unexpected AN received");
		}
		_waiting_for_ack = false;
		_retry_buffer_full = false;
		free(_retry_packet);
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
	
	bool is_data = ((packet[0] & 0x1) == 1); // data or control?
	bool rc = true;

	if ( is_data ) { // DATA
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
	if (!rc) {
		WARNNL("SteamLinkGeneric::generic_send: send failed!!");
	}
	return rc;
}
