cmake_minimum_required(VERSION 3.19)
project(Slonyara LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools)

qt_standard_project_setup()

qt_add_executable(Slonyara
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    .gitignore
)

qt_add_translations(
    TARGETS Slonyara
    TS_FILES Slonyara_en_US.ts
)

target_link_libraries(Slonyara
    PRIVATE
        Qt::Core
        Qt::Widgets
)

if(WIN32)
    target_link_libraries(Slonyara PRIVATE dwmapi)
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
