#ifndef __BFQ_H__
#define __BFQ_H__
#include <stdint.h>
#include "bloomfilter.h"

class BFQ {
	public:
		BFQ(uint32_t bfs_S, uint32_t hashes_K, uint32_t cells_M, uint32_t pktblk_N);
		~BFQ();
		void add(const Pkt_digest& pd);
		bool contains(const Pkt_digest& pd);
		bool quick_contains(const Pkt_digest& pd);
		bool contains_then_update(const Pkt_digest& pd);
		
	private:	
		void stepping();
		uint32_t m_s, m_k, m_m, m_n;
		std::vector<BloomFilter> bfs;
		uint32_t pkt_cnt, current_bf_idx;
};

#endif
