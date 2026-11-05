#!/usr/bin/env bash
# Build the standard C++ runtime with the same MSan instrumentation as the tests.
set -euo pipefail

work_dir="${1:?usage: build-msan-libcxx.sh WORK_DIR}"
mkdir -p "$work_dir"
work_dir="$(cd "$work_dir" && pwd)"
source_dir="$work_dir/llvm-project-18.1.8.src"
if [[ ! -d "$source_dir" ]]; then
    curl --fail --location --retry 3 \
        https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/llvm-project-18.1.8.src.tar.xz \
        --output "$work_dir/llvm.tar.xz"
    tar -xf "$work_dir/llvm.tar.xz" -C "$work_dir"
fi
cmake -S "$source_dir/runtimes" -B "$work_dir/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=clang-18 \
    -DCMAKE_CXX_COMPILER=clang++-18 \
    -DCMAKE_INSTALL_PREFIX="$work_dir/prefix" \
    -DLLVM_ENABLE_RUNTIMES='libcxx;libcxxabi;libunwind' \
    -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF \
    -DLLVM_USE_SANITIZER=MemoryWithOrigins \
    -DLIBCXX_ENABLE_SHARED=OFF \
    -DLIBCXX_ENABLE_STATIC=ON \
    -DLIBCXX_ENABLE_TESTS=OFF \
    -DLIBCXX_STATICALLY_LINK_ABI_IN_STATIC_LIBRARY=ON \
    -DLIBCXXABI_ENABLE_SHARED=OFF \
    -DLIBCXXABI_ENABLE_STATIC=ON \
    -DLIBCXXABI_ENABLE_TESTS=OFF \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DLIBUNWIND_ENABLE_SHARED=OFF \
    -DLIBUNWIND_ENABLE_STATIC=ON \
    -DLIBUNWIND_ENABLE_TESTS=OFF
cmake --build "$work_dir/build" --parallel 2
cmake --install "$work_dir/build"
