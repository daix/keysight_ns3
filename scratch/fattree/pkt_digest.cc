#include "pkt_digest.h"


Pkt_digest::Pkt_digest():zero_padding(0), extended(0) {}; 

bool Pkt_digest::operator== (const Pkt_digest &pd) const {
	return ((srcAddr == pd.srcAddr) && (dstAddr == pd.dstAddr) && (srcPort == pd.srcPort) && (dstPort == pd.dstPort) && (proto == pd.proto));
};

void Pkt_digest::Print(std::ostream& out){
	out << "srcAddr: " << srcAddr \
	<< "  dstAddr: " << dstAddr \
	<< "  srcPort: " << srcPort \
	<< "  dstPort: " << dstPort \
	<< "  protocol: " << (uint16_t)proto \
	<< "  extended: " << extended << std::endl;
}
