// Fuzz the --params-file path: parse_config (config tokenizer) on the input file, then
// Params(args) (every option's process_arg: hosts/ports, socks5, onion/i2p, wallets).
// DNS is disabled globally in fuzz_global_init, so hosts never trigger a network lookup.
// Params::process_arg also writes process-wide logging globals (--loglevel -> log::GLOBAL_LOG_LEVEL,
// --no-color -> log::CONSOLE_COLORS); they are saved before and restored after every input so each
// input's result depends only on that input. (--no-dns sets disable_resolve_host, already pinned true.)
#include "fuzz_common.h"
#include "params.h"
#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

using namespace p2pool;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	fuzz_global_init();
	const char* tmp = getenv("TMPDIR");
	std::string path = std::string(tmp && *tmp ? tmp : "/tmp") + "/p2pool_params_XXXXXX";
	const int fd = mkstemp(&path[0]);
	if (fd < 0) return 0;
	size_t off = 0;
	while (off < size) {
		const ssize_t k = write(fd, data + off, size - off);
		if (k <= 0) break;
		off += static_cast<size_t>(k);
	}
	close(fd);
	const int saved_level = log::GLOBAL_LOG_LEVEL.load();
	const bool saved_colors = log::CONSOLE_COLORS;
	try {
		Params p(parse_config(path));
		(void)p.valid();
	}
	catch (...) {}
	log::GLOBAL_LOG_LEVEL.store(saved_level);
	log::CONSOLE_COLORS = saved_colors;
	disable_resolve_host = true;
	unlink(path.c_str());
	return 0;
}
