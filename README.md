# Slonyara

Requires Qt 6.9+, CMake 3.21+, Python 3, and PostgreSQL development libraries.
On Windows, also install the matching 64-bit MinGW kit and Ninja.

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
