#ifndef __PKT_DB_H__
#define __PKT_DB_H__
#include "pkt_digest.h"
#include "hash.h"
#include <unordered_set>

struct Pkt_digest_hash
{
    size_t operator()(const Pkt_digest& pd) const
    {
        //return pd.srcAddr;
	return hash_crc32(&pd, PKT_DG_LEN, 0);//maybe slow
    }
};
class Pkt_database {
	public:
		Pkt_database(); 
		void add(const Pkt_digest& pd);
		bool contains(const Pkt_digest& pd);
	private:
		std::unordered_set<Pkt_digest, Pkt_digest_hash> db;
};

Pkt_database::Pkt_database(){}

void Pkt_database::add(const Pkt_digest& pd){
	db.insert(pd);	
}

bool Pkt_database::contains(const Pkt_digest& pd){
	return db.find(pd) != db.end();
}
#endif
