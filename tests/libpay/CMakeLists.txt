set(STORE_TEMPLATE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/com_canonical_pay_store.py")
add_definitions(-DSTORE_TEMPLATE_PATH="${STORE_TEMPLATE_FILE}")

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
