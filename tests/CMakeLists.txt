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
# staging script
#############################

install(PROGRAMS setup-staging.sh DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

#############################
# dbusmock template
#############################

set(STORE_TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/com_canonical_pay_store.py")
add_definitions(-DSTORE_TEMPLATE_PATH="${STORE_TEMPLATE_FILE}")

# Get the python3 package directory
execute_process (
  COMMAND python3 -c "from distutils import sysconfig; print(sysconfig.get_python_lib())"
  COMMAND sed -r -e "s|/usr/(local/)?||g"
  OUTPUT_VARIABLE PYTHON_PACKAGE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(
  FILES ${STORE_TEMPLATE_FILE}
  DESTINATION "${PYTHON_PACKAGE_DIR}/dbusmock/templates/"
)

#############################
# libpay tests
#############################

function(add_test_by_name name)
  set(TEST_NAME ${name})
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp ${libpay-tests-generated})
  target_link_libraries(${TEST_NAME} libpay ${GMOCK_BOTH_LIBRARIES})
  add_test(${TEST_NAME} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
endfunction()
add_test_by_name(libpay-iap-tests)
add_test_by_name(libpay-package-tests)
