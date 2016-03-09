# Need xvfb-run for some tests.
set(XVFB_CMD xvfb-run -a -s "-screen 0 540x960x24")

# Standard install paths
set(APP_NAME payui)

set(QT_IMPORTS_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/payui")
set(PAYUI_DIR "${CMAKE_INSTALL_FULL_DATADIR}/payui/qml")

add_subdirectory(app)
add_subdirectory(backend)

configure_file(pay-ui.in pay-ui)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/pay-ui
  DESTINATION lib/payui
)

add_custom_target("autopilot"
  COMMAND PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests/autopilot BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR} SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} U1_DEBUG=1 ${XVFB_CMD} ${CMAKE_CURRENT_SOURCE_DIR}/tests/autopilot/run_autopilot
  DEPENDS payuibackend payuibackend-qmldir
)
