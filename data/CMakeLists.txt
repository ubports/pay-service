
##############################
# Upstart Job
##############################

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pay-service.conf.in ${CMAKE_CURRENT_BINARY_DIR}/pay-service.conf @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pay-service.conf
        DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/upstart/sessions/)

