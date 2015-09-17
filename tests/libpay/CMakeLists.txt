add_definitions(-DSTORE_TEMPLATE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/store.py")

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
