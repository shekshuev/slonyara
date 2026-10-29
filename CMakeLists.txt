cmake_minimum_required(VERSION 3.19)
project(Slonyara LANGUAGES CXX)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Widgets LinguistTools)

file(GLOB_RECURSE SLONYARA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
)

if(WIN32)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(mac|linux)\\.(cpp|mm)$")
elseif(APPLE)
    enable_language(OBJCXX)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(win|linux)\\.(cpp|mm)$")
else()
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "_(win|mac)\\.(cpp|mm)$")
endif()

if(NOT APPLE)
    list(FILTER SLONYARA_SOURCES EXCLUDE REGEX "\\.mm$")
endif()

qt_standard_project_setup()

qt_add_executable(Slonyara
    WIN32 MACOSX_BUNDLE
    ${SLONYARA_SOURCES}
)

qt_add_translations(
    TARGETS Slonyara
    TS_FILES resources/translations/Slonyara_en_US.ts
)

target_link_libraries(Slonyara
    PRIVATE
        Qt::Core
        Qt::Widgets
)

if(WIN32)
    target_link_libraries(Slonyara PRIVATE dwmapi user32)
elseif(APPLE)
    find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(Slonyara PRIVATE ${APPKIT_FRAMEWORK} ${QUARTZCORE_FRAMEWORK})
    set_target_properties(Slonyara PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES)
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
