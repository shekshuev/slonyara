# Slonyara

Requires Qt 6.9+, CMake 3.21+, Python 3, and PostgreSQL development libraries.
On Windows, also install the matching 64-bit MinGW kit and Ninja.

Fetch vendored libraries: `git submodule update --init --recursive`.
On Linux, QtKeychain also needs Qt DBus and libsecret development packages.

## Windows

Copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and adjust the paths.
In Qt Creator, use **Build > Reload CMake Presets** and select
**Windows / MinGW / Debug (local paths)**. In Run Settings, select the build
environment as the base environment so PostgreSQL DLLs can be found.

Or build from the command line:

```powershell
cmake --preset windows-local-debug
cmake --build --preset windows-local-debug
```

[libpqxx build documentation](https://github.com/jtv/libpqxx/blob/master/BUILDING-cmake.md)

## Tests

Enable Qt Test tests in your configured build directory:

```sh
cmake -S . -B <build-dir> -DSLONYARA_BUILD_TESTS=ON
cmake --build <build-dir>
ctest --test-dir <build-dir> --output-on-failure -L slonyara
```

Fast tests use temporary settings and fake Keychain/DB operations; PostgreSQL integration setup is in [tests/README.md](tests/README.md).

## Checks

Use clang-format 18.1.8, clang-tidy 18 and Clazy; configure analysis builds with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and build before running analysis.

- Formatting: run `python3 tools/format.py --check` to check, or `python3 tools/format.py` to apply the project's Qt style.
- C++ lint: run `python3 tools/lint.py <build-dir> --checks=-clang-analyzer-*`.
- Qt lint: run `python3 tools/lint.py <build-dir> --tool clazy`.
- SAST: run `python3 tools/lint.py <build-dir> --checks='-*,clang-analyzer-*,-clang-analyzer-optin.*'`.
- Compiler warnings: configure with `-DSLONYARA_WARNINGS_AS_ERRORS=ON` and run `cmake --build <build-dir>`.
- Tests: run `ctest --test-dir <build-dir> --output-on-failure -L slonyara`, adding `-LE integration` for fast tests only.
- ASan/UBSan: configure a separate application build with `-DSLONYARA_BUILD_TESTS=ON -DSLONYARA_SANITIZER=address-undefined`, then build and run `ctest --test-dir <asan-build> --output-on-failure -L slonyara`.
- TSan (Clang): run `cmake -S tests/sanitizers -B build/tsan -DSLONYARA_CORE_SANITIZER=thread`, then `cmake --build build/tsan` and `ctest --test-dir build/tsan --output-on-failure`.
- MSan (Linux, Clang 18): run `bash tools/build-msan-libcxx.sh build/msan-libcxx`, configure with `cmake -S tests/sanitizers -B build/msan -DCMAKE_CXX_COMPILER=clang++-18 -DSLONYARA_CORE_SANITIZER=memory -DSLONYARA_MSAN_LIBCXX="$PWD/build/msan-libcxx/prefix"`, then `cmake --build build/msan` and `ctest --test-dir build/msan --output-on-failure`.

TSan/MSan cover the production encoding/cancellation core without Qt; disable Qt Creator's **Use custom settings** to honor `.clang-format`.

