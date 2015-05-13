set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")
set (GMOCK_BOTH_LIBRARIES gmock gmock_main)
add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

find_package(Threads)

add_definitions(-DCORE_DBUS_ENABLE_GOOGLE_TEST_FIXTURE)

include_directories(
  ${CMAKE_SOURCE_DIR}
  "${CMAKE_SOURCE_DIR}/common"
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_BINARY_DIR}

  ${GMOCK_INCLUDE_DIR}
  ${GTEST_INCLUDE_DIR}
)

#############################
# DBus Interface
#############################

add_executable(
  dbus-interface-tests
  dbus-interface-tests.cpp
  )

target_link_libraries(
  dbus-interface-tests

  pay-service-lib

  ${SERVICE_DEPS_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(dbus-interface-tests ${CMAKE_CURRENT_BINARY_DIR}/dbus-interface-tests)

#############################
# Memory Item Store
#############################

add_executable(
  item-memory-tests
  item-memory-tests.cpp
  )

target_link_libraries(
  item-memory-tests

  pay-service-lib

  ${SERVICE_DEPS_LIBRARIES}
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(item-memory-tests ${CMAKE_CURRENT_BINARY_DIR}/item-memory-tests)

#############################
# Webclient Curl Factory
#############################

add_executable(
  webclient-curl-tests
  webclient-curl-tests.cpp
)

target_link_libraries(
  webclient-curl-tests

  pay-service-lib

  ${SERVICE_DEPS_LIBRARIES}
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(webclient-curl-tests ${CMAKE_CURRENT_BINARY_DIR}/webclient-curl-tests)

#############################
# Verification HTTP Factory
#############################

add_definitions(-DVERIFICATION_CURL_ENDPOINTS_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/verification-http-endpoints\")

add_executable(
  verification-http-tests
  verification-http-tests.cpp
  )

target_link_libraries(
  verification-http-tests

  pay-service-lib

  ${SERVICE_DEPS_LIBRARIES}
  ${GMOCK_BOTH_LIBRARIES}
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verification-http-endpoints/good/simple.in
  ${CMAKE_CURRENT_BINARY_DIR}/verification-http-endpoints/good/simple @ONLY)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verification-http-endpoints/good/device-id.in
  ${CMAKE_CURRENT_BINARY_DIR}/verification-http-endpoints/good/device-id @ONLY)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/verification-http-endpoints/package-name.in
  ${CMAKE_CURRENT_BINARY_DIR}/verification-http-endpoints/package-name @ONLY)

add_test(verification-http-tests ${CMAKE_CURRENT_BINARY_DIR}/verification-http-tests)

#############################
# Purchase UAL
#############################

add_definitions(-DCMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DCMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(
  purchase-ual-tests
  purchase-ual-tests.cpp
  mir-mock.cpp
  )

target_link_libraries(
  purchase-ual-tests

  pay-service-lib
  ${SERVICE_DEPS_LIBRARIES}
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(purchase-ual-tests ${CMAKE_CURRENT_BINARY_DIR}/purchase-ual-tests)

#############################
# libpay Tests
#############################

add_executable(
  libpay-tests
  libpay-tests.cpp
  )

target_link_libraries(
  libpay-tests

  libpay
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(libpay-tests ${CMAKE_CURRENT_BINARY_DIR}/libpay-tests)

#############################
# staging script
#############################

install(PROGRAMS setup-staging.sh DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})
