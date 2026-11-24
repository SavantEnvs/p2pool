// Fuzz PoolBlock::deserialize — the sidechain block wire format every P2P block message
// (BLOCK_RESPONSE / BLOCK_BROADCAST / BLOCK_BROADCAST_COMPACT) and the block cache go through.
// Byte 0 selects the deserializer mode the P2P server uses (compact / allow_pruned); the rest is the blob.
// Kept across inputs on purpose: one Mainnet "default" SideChain, only read (const&) by deserialize.
#include "fuzz_common.h"

using namespace p2pool;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	fuzz_global_init();
	static SideChain* sidechain = new SideChain(nullptr, NetworkType::Mainnet, "default");
	if (size < 1) return -1;
	const bool compact = data[0] & 1;
	const bool allow_pruned = data[0] & 2;

	PoolBlock b;
	if (b.deserialize(data + 1, size - 1, *sidechain, compact, allow_pruned) == 0) {
		(void)b.serialize_mainchain_data();
		(void)b.serialize_sidechain_data();
		(void)b.get_full_id();
	}
	return 0;
}
