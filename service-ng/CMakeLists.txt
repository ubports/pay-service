
# Here be Go
add_custom_target(pay-service-ng ALL
  COMMAND GOPATH=${CMAKE_CURRENT_SOURCE_DIR} go build pay-service-ng
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/pay-service-ng
  DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}
)
