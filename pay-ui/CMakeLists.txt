# Standard install paths
set(APP_NAME payui)

set(QT_IMPORTS_DIR "${CMAKE_INSTALL_LIBEXECDIR}/payui")
set(PAYUI_DIR "${CMAKE_INSTALL_DATADIR}/payui/qml")

add_subdirectory(app)
add_subdirectory(backend)

configure_file(pay-ui.in pay-ui)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/pay-ui
  DESTINATION ${QT_IMPORTS_DIR}
)

add_custom_target("autopilot"
  COMMAND PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/autopilot BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR} SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests/autopilot/run_autopilot
  DEPENDS payuibackend payuibackend-qmldir
  )
