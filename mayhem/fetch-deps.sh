#!/usr/bin/env bash
# Fetch + build p2pool's submodule dependencies into /opt/toolchains/deps (SPEC §6.2 item 19).
# Pinned to the gitlink commits recorded in the upstream tree; verified by sha256.
# Dependencies stay uninstrumented and carry -g -gdwarf-3.
set -euo pipefail
D=/opt/toolchains/deps
fetch() { # name repo commit sha256
  local name=$1 repo=$2 commit=$3 sum=$4 d="$D/$1-${3:0:12}"
  mkdir -p "$d/src"
  curl -fsSL -o /tmp/dep.tgz "https://codeload.github.com/$repo/tar.gz/$commit"
  echo "$sum  /tmp/dep.tgz" | sha256sum -c -
  tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d/src"
  rm -f /tmp/dep.tgz
  ln -sfn "$d" "$D/$name"
}
fetch randomx   SChernykh/RandomX            12bd428622c103787449f607b19d459a78aabea8 59b86ebba7e93aa317be0d07a57a0760f53ca1520cfa2500a4e2b0d3829998ec
fetch rapidjson SChernykh/rapidjson          43a1db7dbab5680b351f723ba3baeb86f7bb0b38 47553548351842e69fa8f6fbf97a4d819d8df33955257492c3b83936a3461dee
fetch robin-hood-hashing SChernykh/robin-hood-hashing cd9ea864b86fcb742e3a0d1c2eebbdf7b0b6af31 1e7c6b7604c97962dd3de06be6455d53d0637064532591dab576cd0bdfa519f7
fetch googletest google/googletest           063de7e9578f82b369302001269680b4b1553359 623fe05d283020f37a579a8985ba338091ac7229da62b5591ff40acdf05b95e2
fetch boringssl google/boringssl             30a26e970e14f9d943e3961de49bee5cc7e032d3 808e43ad32491852e633178448f9f1e1eec71daf2dac6871123d52a5062f19f5
# BoringSSL: only the sources p2pool compiles itself (ed25519 + sha) are needed; drop the rest.
( cd "$D/boringssl/src" && find . -mindepth 1 -maxdepth 1 ! -name crypto ! -name include ! -name third_party ! -name LICENSE -exec rm -rf {} + )
# Static libs, out of tree.
for p in randomx:"-DARCH=default" googletest:"-DBUILD_GMOCK=OFF -DINSTALL_GTEST=ON"; do
  n=${p%%:*} extra=${p#*:}
  cmake -S "$D/$n/src" -B /tmp/b-$n -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$(readlink -f $D/$n)" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_C_FLAGS="-O2 -g -gdwarf-3" -DCMAKE_CXX_FLAGS="-O2 -g -gdwarf-3" $extra
  cmake --build /tmp/b-$n -j"$(nproc)"
  if [ "$n" = randomx ]; then mkdir -p "$D/$n/lib" && cp /tmp/b-$n/librandomx.a "$D/$n/lib/"; else cmake --install /tmp/b-$n; fi
  rm -rf /tmp/b-$n
done
# keccak.cpp includes "RandomX/src/cpu.hpp" relative to external/src: give it that layout.
mkdir -p "$D/p2pool-include" && ln -sfn ../randomx/src "$D/p2pool-include/RandomX"
chmod -R a+rX "$D"
