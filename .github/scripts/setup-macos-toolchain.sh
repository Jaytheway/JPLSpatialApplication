#!/usr/bin/env bash
set -eo pipefail

brew install llvm@20
llvm_prefix="$(brew --prefix llvm@20)"
export CC="$llvm_prefix/bin/clang"
export CXX="$llvm_prefix/bin/clang++"
export SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
export MACOSX_DEPLOYMENT_TARGET=12.5
# Homebrew's runtime requires macOS 15. Build only the runtimes for Monterey.
runtime_prefix="$RUNNER_TEMP/llvm-runtime"
curl --fail --location --retry 3 \
    https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/llvm-project-20.1.8.src.tar.xz \
    -o "$RUNNER_TEMP/llvm-source.tar.xz"
tar -xf "$RUNNER_TEMP/llvm-source.tar.xz" -C "$RUNNER_TEMP"
cmake -S "$RUNNER_TEMP/llvm-project-20.1.8.src/runtimes" \
    -B "$RUNNER_TEMP/llvm-runtime-build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$runtime_prefix" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.5 \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF \
    -DLLVM_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
    -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
    -DCMAKE_SHARED_LINKER_FLAGS="-Wl,-headerpad_max_install_names"
cmake --build "$RUNNER_TEMP/llvm-runtime-build" --parallel 3
cmake --install "$RUNNER_TEMP/llvm-runtime-build"
for component in libcxx libcxxabi libunwind; do
    mkdir -p "$runtime_prefix/share/licenses/$component"
    cp "$RUNNER_TEMP/llvm-project-20.1.8.src/$component/LICENSE.TXT" \
        "$runtime_prefix/share/licenses/$component/"
done
# Use the headers and libraries from the same runtime build.
export CXXFLAGS="-stdlib=libc++ -nostdinc++ -isystem $runtime_prefix/include/c++/v1"
export LDFLAGS="-L$runtime_prefix/lib -Wl,-rpath,$runtime_prefix/lib -Wl,-headerpad_max_install_names -lunwind"
{
    echo "CC=$CC"
    echo "LLVM_ROOT=$runtime_prefix"
    echo "CXX=$CXX"
    echo "SDKROOT=$SDKROOT"
    echo "MACOSX_DEPLOYMENT_TARGET=$MACOSX_DEPLOYMENT_TARGET"
    echo "CXXFLAGS=$CXXFLAGS"
    echo "LDFLAGS=$LDFLAGS"
} >> "$GITHUB_ENV"
"$CXX" --version
