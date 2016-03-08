set(XVFB_CMD xvfb-run -a -s "-screen 0 640x480x24")

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../payui)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

set(PURCHASE_TESTS_TARGET test-purchase-ui)

add_executable(${PURCHASE_TESTS_TARGET}
    test_network.cpp
)
target_link_libraries(${PURCHASE_TESTS_TARGET}
    -Wl,-rpath,${CMAKE_CURRENT_BINARY_DIR}/../payui
    -L${CMAKE_CURRENT_BINARY_DIR}/../payui
    ${PAYUI_BACKEND}
)
qt5_use_modules(${PURCHASE_TESTS_TARGET} Qml Quick Core DBus Xml Network Test)

add_custom_target(payui-cppunit-tests
  COMMAND env LANGUAGE=en_US.UTF-8 LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8 ${XVFB_CMD} ${CMAKE_CURRENT_BINARY_DIR}/${PURCHASE_TESTS_TARGET}
  DEPENDS ${PURCHASE_TESTS_TARGET} mock_click_server.py
)

add_test(NAME payui-cppunit-tests
  COMMAND make payui-cppunit-tests
)

add_custom_target(mock_click_server.py
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/mock_click_server.py ${CMAKE_CURRENT_BINARY_DIR}/mock_click_server.py
)
