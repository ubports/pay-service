file(GLOB QML_JS_FILES *.qml *.js)

install(FILES ${QML_JS_FILES} DESTINATION ${PAYUI_DIR})

add_subdirectory(components)
add_subdirectory(ui)
