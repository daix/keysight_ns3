#include "bloomfilter.h"
#include <iostream>
#include "hash.h"

BloomFilter::BloomFilter(uint32_t hashes_K, uint32_t cells_M)
:m_numHashes(hashes_K),
m_numCells(cells_M)
{
    	assert(("cannot have more than 8 hash functions", m_numHashes < CRC32_ALG_NUM && m_numHashes >= 0));
	m_bits.resize(m_numHashes);	
	for(uint32_t i = 0; i < m_numHashes; ++i)
		m_bits[i].resize(m_numCells);
}
BloomFilter::~BloomFilter(){
}


void BloomFilter::clear(){
	for(uint32_t i = 0; i < m_numHashes; ++i)
		std::fill(m_bits[i].begin(), m_bits[i].end(), false);
}


void BloomFilter::add(const void *data, std::size_t len){
	for(uint32_t i = 0; i < m_numHashes; ++i){
		m_bits[i][hash_crc32(data, len, i) % m_numCells] = true;
	}
}

bool BloomFilter::contains(const void *data, std::size_t len){
	for(uint32_t i = 0; i < m_numHashes; ++i){
		if(!m_bits[i][hash_crc32(data, len, i) % m_numCells])
			return false;
	}
	return true;
}

bool BloomFilter::contains_with_hashvl(const std::vector<uint32_t>& hvv){
	for(uint32_t i = 0; i < m_numHashes; ++i){
		if(!m_bits[i][hvv[i]])
			return false;
	}
	return true;
}
bool BloomFilter::contains_then_update(const void *data, std::size_t len){
	bool flag = true;
	for(uint32_t i = 0; i < m_numHashes; ++i){
		uint32_t hv = hash_crc32(data, len, i) % m_numCells;
		if(!m_bits[i][hv]){
			m_bits[i][hv] = true;	
			flag = false;
		}
	}
	return flag;
}

void BloomFilter::add(const Pkt_digest& pd){
	add(&pd, PKT_DG_LEN);
}
bool BloomFilter::contains(const Pkt_digest& pd){
	return contains(&pd, PKT_DG_LEN);
}
bool BloomFilter::contains_then_update(const Pkt_digest& pd){
	return contains_then_update(&pd, PKT_DG_LEN);
}
