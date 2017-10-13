# Here be Go

set(SERVICE_NG_TARGET pay-service-2)

# Set environment for go
find_package(Threads)
execute_process(
  COMMAND ${CMAKE_CXX_COMPILER} --print-file-name libstdc++.so
  OUTPUT_VARIABLE LIBSTDC_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND dirname ${LIBSTDC_PATH}
  OUTPUT_VARIABLE GCC_LIB_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(GO_ENV CXX=${CMAKE_CXX_COMPILER} GOPATH=${CMAKE_CURRENT_SOURCE_DIR} CGO_CFLAGS="${SERVICE_DEPS_CFLAGS}" CGO_CXXFLAGS="${SERVICE_DEPS_CFLAGS} -std=c++11" CGO_LDFLAGS="${SERVICE_DEPS_LDFLAGS};-L${GCC_LIB_DIR};${CMAKE_THREAD_LIBS_INIT}")

add_custom_target(${SERVICE_NG_TARGET} ALL
  COMMAND ${GO_ENV} go build ${SERVICE_NG_TARGET}
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_NG_TARGET}
  DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)

set(TEST_TYPE "normal")
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "coverage")
  set(TEST_TYPE "coverage")
endif()

add_custom_target(test-service-ng
  COMMAND ${GO_ENV} dbus-test-runner -m 600 -t ${CMAKE_CURRENT_SOURCE_DIR}/test-service.sh -p ${SERVICE_NG_TARGET}/service -p ${TEST_TYPE} || true
  DEPENDS ${SERVICE_NG_TARGET}
)

add_test(NAME test-service-ng
  COMMAND make test-service-ng
)
