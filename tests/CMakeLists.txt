qt_add_executable(ApplicationControllerTest applicationcontrollertest.cpp)
target_link_libraries(ApplicationControllerTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME ApplicationController COMMAND ApplicationControllerTest)
set_tests_properties(ApplicationController PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

qt_add_executable(ConnectionStoreTest connectionstoretest.cpp)
target_link_libraries(ConnectionStoreTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME ConnectionStore COMMAND ConnectionStoreTest)
set_tests_properties(ConnectionStore PROPERTIES TIMEOUT 15)
set_tests_properties(ApplicationController ConnectionStore PROPERTIES LABELS "slonyara")

option(SLONYARA_BUILD_INTEGRATION_TESTS "Build PostgreSQL integration tests" OFF)
if(SLONYARA_BUILD_INTEGRATION_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Network)
    qt_add_executable(ConnectionTesterTest connectiontestertest.cpp)
    target_link_libraries(ConnectionTesterTest PRIVATE SlonyaraCore Qt6::Test Qt6::Network)
    add_test(NAME ConnectionTester COMMAND ConnectionTesterTest)
    set_tests_properties(ConnectionTester PROPERTIES LABELS "slonyara;integration" TIMEOUT 60)

    qt_add_executable(ConnectionWorkerTest connectionworkertest.cpp)
    target_link_libraries(ConnectionWorkerTest PRIVATE SlonyaraCore Qt6::Test Qt6::Network)
    add_test(NAME ConnectionWorker COMMAND ConnectionWorkerTest)
    set_tests_properties(ConnectionWorker PROPERTIES
        LABELS "slonyara;integration"
        TIMEOUT 90
    )
endif()

qt_add_executable(StatusDetailsDialogTest statusdetailsdialogtest.cpp)
target_link_libraries(StatusDetailsDialogTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME StatusDetailsDialog COMMAND StatusDetailsDialogTest)
set_tests_properties(StatusDetailsDialog PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "slonyara"
    TIMEOUT 30
)

qt_add_executable(ConnectionDialogTest connectiondialogtest.cpp)
target_link_libraries(ConnectionDialogTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME ConnectionDialog COMMAND ConnectionDialogTest)
set_tests_properties(ConnectionDialog PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "slonyara"
    TIMEOUT 30
)

qt_add_executable(StatusCardTest statuscardtest.cpp)
target_link_libraries(StatusCardTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME StatusCard COMMAND StatusCardTest)
set_tests_properties(StatusCard PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "slonyara"
    TIMEOUT 15
)

qt_add_executable(IntroWindowTest introwindowtest.cpp)
target_link_libraries(IntroWindowTest PRIVATE SlonyaraCore Qt6::Test)
add_test(NAME IntroWindow COMMAND IntroWindowTest)
set_tests_properties(IntroWindow PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "slonyara"
    TIMEOUT 30
)
