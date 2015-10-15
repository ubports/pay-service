# Here be Go

set(SERVICE_NG_TARGET pay-service-2)

# Set environment for go
set(GO_ENV GOPATH=${CMAKE_CURRENT_SOURCE_DIR} CGO_CFLAGS="${SERVICE_DEPS_CFLAGS}" CGO_CXXFLAGS="${SERVICE_DEPS_CFLAGS}" CGO_LDFLAGS="${SERVICE_DEPS_LDFLAGS}")

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
  COMMAND ${GO_ENV} dbus-test-runner -m 600 -t ${CMAKE_CURRENT_SOURCE_DIR}/test-service.sh -p ${SERVICE_NG_TARGET}/service -p ${TEST_TYPE}
  DEPENDS ${SERVICE_NG_TARGET}
)

add_test(NAME test-service-ng
  COMMAND make test-service-ng
)
