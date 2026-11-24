// Shared setup for p2pool harnesses: the link stubs upstream's tests/src/main.cpp provides for the
// P2POOL_UNIT_TESTS build, plus the one-time global init main() does.
#pragma once
#include "common.h"
#include "util.h"
#include "pool_block.h"
#include "side_chain.h"
#include <cstddef>
#include <cstdint>

namespace p2pool { void fuzz_global_init(); }
