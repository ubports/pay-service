set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

find_package(Qt5Core)
find_package(Qt5Qml)
find_package(Qt5Quick)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

pkg_check_modules(UBUNTUONEAUTH REQUIRED ubuntuoneauth-2.0)
add_definitions(${UBUNTUONEAUTH_CFLAGS} ${UBUNTUONEAUTH_CFLAGS_OTHER})

set(
    payuibackend_SRCS
    modules/payui/backend.cpp
    modules/payui/purchase.cpp
    modules/payui/network.cpp
    modules/payui/pay_info.cpp
    modules/payui/credentials_service.cpp
    modules/payui/certificateadapter.cpp
    modules/payui/oxideconstants.cpp
)
set(PAYUI_BACKEND payuibackend)

add_library(${PAYUI_BACKEND} SHARED
    ${payuibackend_SRCS}
)

target_link_libraries(${PAYUI_BACKEND}
    ${UBUNTUONEAUTH_LDFLAGS}
)

set_target_properties(${PAYUI_BACKEND} PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY payui)

qt5_use_modules(${PAYUI_BACKEND} Gui Qml Quick Network DBus)

# Copy qmldir file to build dir for running in QtCreator
add_custom_target(payuibackend-qmldir ALL
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/modules/payui/qmldir ${CMAKE_CURRENT_BINARY_DIR}/payui
    DEPENDS ${QMLFILES}
)

# Install plugin file
install(TARGETS ${PAYUI_BACKEND} DESTINATION ${QT_IMPORTS_DIR}/payui/)
install(FILES   modules/payui/qmldir DESTINATION ${QT_IMPORTS_DIR}/payui/)

add_subdirectory (tests)
