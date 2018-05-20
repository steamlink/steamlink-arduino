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
			#ifndef UNIX
        	Serial.print(hbuf);
        	Serial.println(sbuf);
			#else
			std::cout<<hbuf;
        	std::cout<<sbuf<<std::endl;
			#endif
        	ih = 0;
    	}
  	}
  	if (ih > 0) {
    	sbuf[ih] = '\0';
    	hbuf[ih*3] = '\0';
		#ifndef UNIX
			Serial.print(hbuf);
			Serial.println(sbuf);
		#else
			std::cout<<hbuf;
			std::cout<<sbuf<<std::endl;
		#endif
  	}
}

void print_packet(uint8_t* packet, uint8_t packet_length) {
	bool is_data = ((packet[0] & 0x1) == 1); // data or control?
	if (is_data) {
		uint8_t op = ((data_header*) packet)->op;
		uint32_t slid = ((data_header*) packet)->slid;
		uint16_t pkg_num = ((data_header*) packet)->pkg_num;
		uint8_t rssi = ((data_header*) packet)->rssi;
		
		INFONL(" ");
		INFONL("|-----------------------------------|");
		INFONL("|         START OF PACKET           |");
		INFONL("|-----------------------------------|");
		INFONL("|           DATA HEADER             |");
		INFONL("|-----------------------------------|");
		INFO  ("| OP      : "); INFOHEX(op); INFO(" <-> "); print_op_code(op); INFONL(" ");
		INFO  ("| SLID    : "); INFO(slid); INFO(" <-> HEX: "); INFOHEX(slid); INFONL(" ");
		INFO  ("| PKG NUM : "); INFONL(pkg_num);
		INFO  ("| RSSI    : "); INFONL(rssi);
		INFONL("|-----------------------------------|");
		#if DEBUG_PACKET_VERBOSE
		uint8_t* payload = packet + sizeof(data_header);
		uint8_t payload_length = packet_length - sizeof(data_header);
		INFONL("|           DATA PAYLOAD            |");
		INFONL"|-----------------------------------|");
		phex(payload, payload_length);
		INFONL"|-----------------------------------|");
		#endif
		INFONL("|           END OF PACKET           |");
		INFONL("|-----------------------------------|");
		INFONL(" ");

	} else {
		uint8_t op = ((control_header*) packet)->op;
		uint32_t slid = ((control_header*) packet)->slid;
		uint16_t pkg_num = ((control_header*) packet)->pkg_num;

		INFONL(" ");
		INFONL("|-----------------------------------|");
		INFONL("|         START OF PACKET           |");
		INFONL("|-----------------------------------|");
		INFONL("|         CONTROL HEADER            |");
		INFONL("|-----------------------------------|");
		INFO  ("| OP      : "); INFOHEX(op); INFO(" <-> "); print_op_code(op); INFONL(" ");
		INFO  ("| SLID    : "); INFO(slid); INFO(" <-> HEX: "); INFOHEX(slid); INFONL(" ");
		INFO  ("| PKG NUM : "); INFONL(pkg_num);
		INFONL("|-----------------------------------|");
		#if DEBUG_PACKET_VERBOSE
		uint8_t* payload = packet + sizeof(control_header);
		uint8_t payload_length = packet_length - sizeof(control_header);
		INFONL("|          CONTROL PAYLOAD          |");
		INFONL("|-----------------------------------|");
		phex(payload, payload_length);
		INFONL("|-----------------------------------|");
		#endif
		INFONL("|           END OF PACKET           |");
		INFONL("|-----------------------------------|");
		INFONL(" ");
	}
}

void print_op_code(uint8_t op) {
	if (op == SL_OP_DN) INFO("DN");
	if (op == SL_OP_BN) INFO("BN");
	if (op == SL_OP_GS) INFO("GS");
	if (op == SL_OP_TD) INFO("TD");
	if (op == SL_OP_SC) INFO("SC");	
	if (op == SL_OP_BC) INFO("BC");
	if (op == SL_OP_BR) INFO("BR");
	if (op == SL_OP_AN) INFO("AN");

	if (op == SL_OP_DS) INFO("DS");
	if (op == SL_OP_BS) INFO("BS");
	if (op == SL_OP_ON) INFO("ON");
	if (op == SL_OP_AS) INFO("AS");
	if (op == SL_OP_MS) INFO("MS");
	if (op == SL_OP_TR) INFO("TR");
	if (op == SL_OP_SS) INFO("SS");
	if (op == SL_OP_OF) INFO("OF");
}

#ifdef UNIX
unsigned long millis() {
	return 0;
}

#endif