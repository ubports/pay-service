project(pay-test-app)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Werror")

include(GNUInstallDirs)

# Workaround for libexecdir on debian
if (EXISTS "/etc/debian_version") 
	set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
	set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
endif()

set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")

find_package(Qt5Core)
find_package(Qt5Qml)
find_package(Qt5Quick)
find_package(PkgConfig)

pkg_check_modules(PAY REQUIRED pay-2)
include_directories (SYSTEM ${PAY_INCLUDE_DIRS})

add_executable(pay-test-app pay-test-app.cpp package.h package.cpp qtquick2applicationviewer.cpp qtquick2applicationviewer.h)
qt5_use_modules(pay-test-app Core Qml Quick)
target_link_libraries(pay-test-app ${PAY_LIBRARIES})
install(TARGETS pay-test-app RUNTIME DESTINATION ${pkglibexecdir})

install(FILES pay-test-app.qml DESTINATION ${pkglibexecdir})
install(FILES pay-test-app.svg DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/")

configure_file("pay-test-app.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/pay-test-app.desktop" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pay-test-app.desktop"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/")
