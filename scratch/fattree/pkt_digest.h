#ifndef __PKT_DIGEST_H__
#define __PKT_DIGEST_H__

/* 5-tuple of a packet
*/
#define PKT_DG_LEN 13
//#define PKT_DG_LEN 16

#include <iostream>

class Pkt_digest {
	public:
		//host-order 32-bit ip address
		uint32_t srcAddr;
		uint32_t dstAddr;
		//port
		uint16_t srcPort;
		uint16_t dstPort;
		uint8_t proto;	
		//address alignment
		uint8_t zero_padding;
		//field for distinguishing by hash value
		uint16_t extended;

		Pkt_digest();
		bool operator== (const Pkt_digest &pd) const;
		void Print(std::ostream&);

};
#endif
