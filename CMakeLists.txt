project (pay-service C CXX)
cmake_minimum_required (VERSION 2.8.9)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (PACKAGE_VERSION "14.04.0")
set (PACKAGE ${CMAKE_PROJECT_NAME})

option (enable_tests "Build the package's automatic tests." ON)

if (${enable_tests})
  enable_testing ()
endif ()

##
##  GNU standard installation directories
## 

include (GNUInstallDirs)
if (EXISTS "/etc/debian_version") # Workaround for libexecdir on debian
  set (CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}")
  set (CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif ()
set (CMAKE_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
set (CMAKE_INSTALL_FULL_PKGLIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")

##
##  Check for prerequisites
##

set(CMAKE_AUTOMOC ON)

find_package (PkgConfig REQUIRED)
find_package (Qt5Core REQUIRED)

pkg_check_modules (SERVICE_DEPS REQUIRED
  Qt5Core
  click-0.4
  dbus-cpp
  dbustest-1
  gio-2.0
  gio-unix-2.0
  properties-cpp
  trust-store
  ubuntu-app-launch-2>=0.5
)
include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})

##
##  Gdbus
##

include (GdbusCodegen)

##
##  custom targets
##

set (ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PACKAGE_VERSION})
add_custom_target (dist
                   COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target (cppcheck COMMAND cppcheck --enable=all -q --error-exitcode=2 --inline-suppr
                   ${CMAKE_SOURCE_DIR}/libpay
                   ${CMAKE_SOURCE_DIR}/tests)

##
##  Actual building
##

set (CC_WARNING_ARGS " -Wall -Wshadow -Wextra -Wunused -Wformat=2 -Wno-missing-field-initializers")
# those GActionEntry structs tickle -Wmissing-field-initializers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Weverything -Wno-c++98-compat -Wno-documentation -Wno-padded")
else()
  set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Wall -Wextra -Wpedantic -Wunused -Wformat=2 ")
endif()
# service/dbus-interface.cpp gives unavoidable warnings in glib code unless we turn this warning off
set (CXX_WARNING_ARGS "${CXX_WARNING_ARGS} -Wno-missing-field-initializers")


##
##  Subdirectories
##

add_subdirectory(common)
add_subdirectory(libpay)
add_subdirectory(service-ng)
add_subdirectory(ual-helper)
add_subdirectory(data)
if (${enable_tests})
  add_subdirectory(tests)
endif ()
add_subdirectory(pay-test-app)
add_subdirectory(po)

##
##  Coverage Reports
##
include(EnableCoverageReport)
set(filter-list)
list(APPEND filter-list "/usr/include")
list(APPEND filter-list "${CMAKE_SOURCE_DIR}/tests/*")
if (NOT ${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  list(APPEND filter-list "${CMAKE_BINARY_DIR}/*")
endif()
ENABLE_COVERAGE_REPORT(
  TARGETS libpay
  TESTS libpay-iap-tests libpay-package-tests
  FILTER ${filter-list}
)
