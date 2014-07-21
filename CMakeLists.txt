project(pay-test-app)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(PkgConfig)

pkg_check_modules(PAY REQUIRED pay-1)
include_directories (SYSTEM ${PAY_INCLUDE_DIRS})

add_executable(pay-test-app pay-test-app.cpp package.h package.cpp)
qt5_use_modules(pay-test-app Core Qml Quick)
target_link_libraries(pay-test-app ${PAY_LIBRARIES})
install(TARGETS pay-test-app RUNTIME DESTINATION "/${CMAKE_INSTALL_BINDIR}")
