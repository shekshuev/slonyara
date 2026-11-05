cmake_minimum_required(VERSION 3.19)
project(Slonyara
    VERSION 0.0.1
    LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Widgets LinguistTools Concurrent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SKIP_BUILD_TEST ON)
if(WIN32)
    set(BUILD_SHARED_LIBS ON)
else()
    set(BUILD_SHARED_LIBS OFF)
endif()
add_subdirectory(vendor/libpqxx build-pqxx)

function(slonyara_add_qtkeychain)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(BUILD_TRANSLATIONS OFF)
    add_subdirectory(vendor/qtkeychain build-qtkeychain)
endfunction()
slonyara_add_qtkeychain()

file(GLOB_RECURSE SLONYARA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/generated/version.h"
    @ONLY
)

if(WIN32)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(mac|linux)\\.(cpp|mm)$")
elseif(APPLE)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(win|linux)\\.(cpp|mm)$")
else()
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(win|mac)\\.(cpp|mm)$")
endif()

if(NOT APPLE)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "\\.mm$")
endif()

qt_standard_project_setup()

list(REMOVE_ITEM SLONYARA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(SlonyaraCore STATIC ${SLONYARA_SOURCES})
target_include_directories(SlonyaraCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

qt_add_executable(Slonyara WIN32 MACOSX_BUNDLE src/main.cpp)
target_link_libraries(Slonyara PRIVATE SlonyaraCore)

target_include_directories(Slonyara PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/src/generated")

qt_add_translations(
    TARGETS Slonyara
    TS_FILES
        resources/translations/Slonyara_en_US.ts
        resources/translations/Slonyara_ru_RU.ts
    RESOURCE_PREFIX "/i18n"
)


qt_add_resources(Slonyara "images"
    PREFIX "/images"
    BASE "resources/assets/images/"
    FILES
        resources/assets/images/logo.png
)

qt_add_resources(Slonyara "fonts"
    PREFIX "/fonts"
    BASE "resources/assets/fonts/"
    FILES
        resources/assets/fonts/Xolonium-Bold.ttf
        resources/assets/fonts/JetBrainsMono-Regular.ttf
        resources/assets/fonts/Manrope-Bold.ttf
        resources/assets/fonts/Manrope-Regular.ttf
)

qt_add_resources(Slonyara "styles"
    PREFIX "/styles"
    BASE "resources/styles/"
    FILES resources/styles/theme.qss
)

target_link_libraries(SlonyaraCore
    PUBLIC
        Qt::Core
        Qt::Widgets
        pqxx
        qt6keychain
        Qt::Concurrent
)

if(WIN32)
    target_link_libraries(SlonyaraCore PUBLIC dwmapi user32 uxtheme)
elseif(APPLE)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(SlonyaraCore PUBLIC ${APPKIT_FRAMEWORK} ${QUARTZCORE_FRAMEWORK})
    set_target_properties(SlonyaraCore PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES)
endif()

install(TARGETS Slonyara
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Slonyara
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

option(SLONYARA_BUILD_TESTS "Build Slonyara tests" OFF)
if(SLONYARA_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
