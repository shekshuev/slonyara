set(SLONYARA_SANITIZER "none" CACHE STRING "Runtime instrumentation: none or address-undefined")
set_property(CACHE SLONYARA_SANITIZER PROPERTY STRINGS none address-undefined)

if(NOT SLONYARA_SANITIZER STREQUAL "none")
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" OR WIN32)
        message(FATAL_ERROR "Sanitizer builds currently require Clang/GCC on Linux or macOS")
    endif()
    if(SLONYARA_SANITIZER STREQUAL "address-undefined")
        # Set before vendor subdirectories so pqxx and QtKeychain are instrumented too.
        add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all
                            -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "Unsupported SLONYARA_SANITIZER: ${SLONYARA_SANITIZER}")
    endif()
endif()
