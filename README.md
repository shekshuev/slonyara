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

The controller tests use fake sessions and password storage, so PostgreSQL and
system Keychain access are not needed at runtime. They run with the Qt
`offscreen` platform and isolated temporary settings.

The `slonyara` label runs all project tests, excluding vendored library tests.
ConnectionStore tests use isolated temporary INI settings and need no GUI.

PostgreSQL worker integration tests are optional: see [tests/README.md](tests/README.md)
for setup, environment variables, and CI requirements.

StatusDetailsDialog tests run offscreen and cover literal text, read-only details,
clipboard copying, dismissal, parent centering, and opening from StatusCard.

ConnectionDialog tests use fake Keychain operations and controllable futures for
connection checks; no real passwords are stored and no DB connections are made.

IntroWindow tests cover filtering, profile activation, busy/error states, and
create/edit flows with temporary settings and password saving disabled.

## Continuous integration

[GitHub Actions](.github/workflows/ci.yml) runs on pull requests and pushes to
`main`. Linux (Qt 6.9.3) builds the application and runs all project tests against
PostgreSQL 16. macOS and Windows build their native implementations and run the
fast tests. Windows uses a consistent MSYS2 MinGW UCRT64 toolchain with Qt 6 and
PostgreSQL client libraries from the same rolling package repository; unlike the
Linux/macOS Qt 6.9.3 jobs, the Windows Qt version follows MSYS2 updates.
CTest JUnit reports and logs are uploaded as artifacts even on test failure.
No repository secrets or access to personal Keychain entries are needed.

Suggested next checks: compiler warnings on project targets, clang-tidy with
Clazy for Qt-specific diagnostics, then ASan/UBSan and coverage. For formatting,
first commit a `.clang-format` matching Qt Creator and pin the formatter version;
then use the same config in the editor and a CI `--dry-run --Werror` check for
`src/` and `tests/`. Vendored and generated sources should be excluded.
