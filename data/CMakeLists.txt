
##############################
# Upstart Jobs
##############################
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/pay-service-trust-stored.conf
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/upstart/sessions/)


# New dbus service file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/com.canonical.payments.service.in
  ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.payments.service
  @ONLY
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/com.canonical.payments.service
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/services/
)
