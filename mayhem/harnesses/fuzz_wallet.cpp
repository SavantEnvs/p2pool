// Fuzz the address decoders p2pool applies to untrusted strings (wallet addresses from the command
// line, Stratum logins and peers; onion v3 / I2P b32 peer addresses): Wallet::decode + encode
// round-trip + torsion_check, from_onion_v3, from_i2p_b32.
#include "fuzz_common.h"
#include "wallet.h"
#include "i2p.h"
#include <string>

using namespace p2pool;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	fuzz_global_init();
	const std::string s(reinterpret_cast<const char*>(data), size);
	Wallet w(nullptr);
	if (w.decode(s.c_str())) {
		(void)w.encode();
		(void)w.torsion_check();
	}
	(void)from_onion_v3(s);
	(void)from_i2p_b32(s);
	return 0;
}
