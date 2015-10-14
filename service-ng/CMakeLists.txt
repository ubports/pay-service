# Here be Go

set(SERVICE_NG_TARGET pay-service-2)

add_custom_target(${SERVICE_NG_TARGET} ALL
  COMMAND GOPATH=${CMAKE_CURRENT_SOURCE_DIR} go build ${SERVICE_NG_TARGET}
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
  COMMAND GOPATH=${CMAKE_CURRENT_SOURCE_DIR} dbus-test-runner -m 600 -t ${CMAKE_CURRENT_SOURCE_DIR}/test-service.sh -p ${SERVICE_NG_TARGET}/service -p ${TEST_TYPE}
  DEPENDS ${SERVICE_NG_TARGET}
)

add_test(NAME test-service-ng
  COMMAND make test-service-ng
)
