#include <SteamLink.h>

void phex(uint8_t *data, unsigned int length) {
	char sbuf[17]; 
	char hbuf[49]; 
	uint8_t ih = 0;

	hbuf[48] = '\0';
	sbuf[16] = '\0';
	for (int i=0; i<length; i++) {
		sprintf(&hbuf[ih*3], "%02x ", (char) data[i]);
		if ((data[i] >= 0x20) && (data[i] <= 0x7f)) {
    		sbuf[ih] = (char) data[i];
		} else {
        	sbuf[ih] = '.';
    	}
    	ih += 1;
    	if ((i % 16) == 15) {
        	Serial.print(hbuf);
        	Serial.println(sbuf);
        	ih = 0;
    	}
  	}
  	if (ih > 0) {
    	sbuf[ih] = '\0';
    	hbuf[ih*3] = '\0';
    	Serial.print(hbuf);
    	Serial.println(sbuf);
  	}
}

void print_packet(uint8_t* packet, uint8_t packet_length) {
	bool is_data = ((packet[0] & 0x1) == 1); // data or control?
	if (is_data) {
		uint8_t op = ((data_header*) packet)->op;
		uint32_t slid = ((data_header*) packet)->slid;
		uint16_t pkg_num = ((data_header*) packet)->pkg_num;
		uint8_t rssi = ((data_header*) packet)->rssi;
		
		Serial.println(" ");
		Serial.println("|-----------------------------------|");
		Serial.println("|         START OF PACKET           |");
		Serial.println("|-----------------------------------|");
		Serial.println("|           DATA HEADER             |");
		Serial.println("|-----------------------------------|");
		Serial.print  ("| OP      : "); Serial.print(op, HEX); Serial.print(" <-> "); print_op_code(op); Serial.println(" ");
		Serial.print  ("| SLID    : "); Serial.print(slid); Serial.print(" <-> HEX: "); Serial.println(slid, HEX);
		Serial.print  ("| PKG NUM : "); Serial.println(pkg_num);
		Serial.print  ("| RSSI    : "); Serial.println(rssi);
		Serial.println("|-----------------------------------|");
		#if DEBUG_PACKET_VERBOSE
		uint8_t* payload = packet + sizeof(data_header);
		uint8_t payload_length = packet_length - sizeof(data_header);
		Serial.println("|           DATA PAYLOAD            |");
		Serial.println("|-----------------------------------|");
		phex(payload, payload_length);
		Serial.println("|-----------------------------------|");
		#endif
		Serial.println("|           END OF PACKET           |");
		Serial.println("|-----------------------------------|");
		Serial.println(" ");

	} else {
		uint8_t op = ((control_header*) packet)->op;
		uint32_t slid = ((control_header*) packet)->slid;
		uint16_t pkg_num = ((control_header*) packet)->pkg_num;

		Serial.println(" ");
		Serial.println("|-----------------------------------|");
		Serial.println("|         START OF PACKET           |");
		Serial.println("|-----------------------------------|");
		Serial.println("|         CONTROL HEADER            |");
		Serial.println("|-----------------------------------|");
		Serial.print  ("| OP      : "); Serial.print(op, HEX); Serial.print(" <-> "); print_op_code(op); Serial.println(" ");
		Serial.print  ("| SLID    : "); Serial.print(slid); Serial.print(" <-> HEX: "); Serial.println(slid, HEX);
		Serial.print  ("| PKG NUM : "); Serial.println(pkg_num);
		Serial.println("|-----------------------------------|");
		#if DEBUG_PACKET_VERBOSE
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		Serial.println("|          CONTROL PAYLOAD          |");
		Serial.println("|-----------------------------------|");
		phex(payload, payload_length);
		Serial.println("|-----------------------------------|");
		#endif
		Serial.println("|           END OF PACKET           |");
		Serial.println("|-----------------------------------|");
		Serial.println(" ");
	}
}

void print_op_code(uint8_t op) {
	if (op == SL_OP_DN) Serial.print("DN");
	if (op == SL_OP_BN) Serial.print("BN");
	if (op == SL_OP_GS) Serial.print("GS");
	if (op == SL_OP_TD) Serial.print("TD");
	if (op == SL_OP_SC) Serial.print("SC");	
	if (op == SL_OP_BC) Serial.print("BC");
	if (op == SL_OP_BR) Serial.print("BR");
	if (op == SL_OP_AN) Serial.print("AN");

	if (op == SL_OP_DS) Serial.print("DS");
	if (op == SL_OP_BS) Serial.print("BS");
	if (op == SL_OP_ON) Serial.print("ON");
	if (op == SL_OP_AS) Serial.print("AS");
	if (op == SL_OP_MS) Serial.print("MS");
	if (op == SL_OP_TR) Serial.print("TR");
	if (op == SL_OP_SS) Serial.print("SS");
	if (op == SL_OP_OF) Serial.print("OF");
}