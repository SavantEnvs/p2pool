#include "fuzz_common.h"
#include "json_rpc_request.h"
#include "merge_mining_client.h"
#include "crypto.h"
#include "thread_pool.h"

void p2pool_usage() {}

namespace p2pool {

IMergeMiningClient* IMergeMiningClient::create(p2pool*, const std::string&, const std::string&, const std::string&) noexcept { return nullptr; }

namespace JSONRPCRequest {
	void Call(const std::string&, int, const std::string&, const std::string&, const std::string&, bool, const std::string&, CallbackBase*, CallbackBase*, uv_loop_t*) {}
}

// Same global init as upstream tests/src/main.cpp + pool_block_tests (crypto cache).
// Everything here is process-global and never changed by an input.
void fuzz_global_init()
{
	static bool done = false;
	if (done) return;
	done = true;
	memset((void*) &robin_hood::SIPHASH_KEY, 0, sizeof(robin_hood::SIPHASH_KEY));
	set_main_thread();
	PoolBlock::s_precalculatedSharesLock = new ReadWriteLock();
	init_crypto_cache();
	thread_pool_init();
	disable_resolve_host = true;  // no DNS lookups from fuzzed host names (Params --no-dns sets the same global)
}

}
