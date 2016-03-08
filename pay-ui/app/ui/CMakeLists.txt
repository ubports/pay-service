file(GLOB UI_QML_JS_FILES *.qml *.js)

# make the files visible in the qtcreator tree
add_custom_target(payui_ui_QMlFiles ALL SOURCES ${UI_QML_JS_FILES})

install(FILES ${UI_QML_JS_FILES} DESTINATION ${PAYUI_DIR}/ui)
