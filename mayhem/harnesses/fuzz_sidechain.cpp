// Fuzz SideChain block ingestion: a stream of [u32 little-endian length][block] records (the format of
// upstream's tests/src/sidechain_dump*.dat), each deserialized and added to a fresh SideChain, as
// upstream's pool_block.verify test does (share verification, difficulty, uncles,
// PPLNS outputs). Byte 0 picks the pool (default/mini/nano, all with hardcoded consensus IDs).
// A new SideChain is built per input, so no chain state carries across inputs.
#include "fuzz_common.h"
#include <vector>
#include <cstring>

using namespace p2pool;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	fuzz_global_init();
	if (size < 1) return -1;
	static const char* pools[3] = { "default", "mini", "nano" };
	const char* pool = pools[data[0] % 3];
	++data; --size;

	SideChain sidechain(nullptr, NetworkType::Mainnet, pool);
	// Main-chain difficulty used by upstream's sidechain test (block 3454976); m_pool is null.
	sidechain.m_testMainChainDiff = difficulty_type(625461936742ULL, 0ULL);

	for (const uint8_t *p = data, *e = data + size; p + sizeof(uint32_t) <= e;) {
		uint32_t n;
		memcpy(&n, p, sizeof(n));
		p += sizeof(n);
		if (n > static_cast<size_t>(e - p)) break;
		PoolBlock* b = new PoolBlock();
		if (b->deserialize(p, n, sidechain, false, false) == 0) {
			// add_block, as upstream's pool_block.verify test does: add_external_block needs a live
			// p2pool instance (m_pool->chainmain_get_by_hash) that the unit-test build cannot provide.
			(void)sidechain.add_block(*b);
		}
		delete b;
		p += n;
	}
	return 0;
}
