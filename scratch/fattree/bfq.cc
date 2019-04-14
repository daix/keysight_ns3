#include "bfq.h"
#include "hash.h"
#include <iostream>

BFQ::BFQ(uint32_t bfs_S, uint32_t hashes_K, uint32_t cells_M, uint32_t pktblk_N)
:m_s(bfs_S), m_k(hashes_K), m_m(cells_M), m_n(pktblk_N), bfs(bfs_S, BloomFilter(hashes_K, cells_M)), pkt_cnt(0), current_bf_idx(0)
{
//	for(uint32_t i = 0; i < m_s; ++i){
//		bfs.push_back(BloomFilter(m_k, m_m));
//	}
}

BFQ::~BFQ(){ }

void BFQ::stepping(){
	pkt_cnt++;
	if (pkt_cnt == m_n){
		pkt_cnt = 0;
		current_bf_idx = (current_bf_idx + 1) % m_s;	
		bfs[current_bf_idx].clear();
	}	
}

void BFQ::add(const Pkt_digest& pd){
	bfs[current_bf_idx].add(pd);	
	stepping();
}

bool BFQ::contains(const Pkt_digest& pd){
	//for (uint32_t i = 0; i < m_s; ++i){
	//	if (bfs[i].contains(pd))
	//		return true;
	//}
	//return false;
	return quick_contains(pd);
}
bool BFQ::quick_contains(const Pkt_digest& pd){
	std::vector<uint32_t> hash_values;	
	for (uint32_t i = 0; i < m_k; ++i)
		hash_values.push_back(hash_crc32(&pd, PKT_DG_LEN, i) % m_m);
	for (uint32_t i = 0; i < m_s; ++i){
		if (bfs[i].contains_with_hashvl(hash_values))
			return true;
	}
	return false;
}
//deprecated
bool BFQ::contains_then_update(const Pkt_digest& pd){
	return true;
}
