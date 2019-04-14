#ifndef BLOOMFILTER_H_
#define BLOOMFILTER_H_
#include <stdint.h>
#include <vector>
#include "pkt_digest.h"

class BloomFilter {
	public:
		BloomFilter(uint32_t hashes_K, uint32_t cells_M);
		~BloomFilter();
		
		void clear();
		void add(const void *data, std::size_t len);
		bool contains(const void *data, std::size_t len);
		bool contains_then_update(const void *data, std::size_t len);
		void add(const Pkt_digest& pd);
		bool contains(const Pkt_digest& pd);
		bool contains_then_update(const Pkt_digest& pd);

		bool contains_with_hashvl(const std::vector<uint32_t>& hvv);
	//private:
		std::vector<std::vector<bool>> m_bits;
		uint32_t m_numHashes;//K
		uint32_t m_numCells;//M
};

#endif
