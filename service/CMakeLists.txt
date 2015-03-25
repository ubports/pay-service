
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -fPIE ${GCOV_FLAGS}")
include_directories(${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_SOURCE_DIR}/common")


######################
# Service Library
######################

set(SERVICE_LIB_SOURCES
    dbus-interface.h
    dbus-interface.cpp
    item-interface.h
    item-null.h
    item-memory.h
    item-memory.cpp
    qtbridge.h
    qtbridge.cpp
    purchase-factory.h
    purchase-null.h
    purchase-null.cpp
    purchase-ual.h
    purchase-ual.cpp
    token-grabber.h
    token-grabber-u1.h
    token-grabber-u1.cpp
    verification-curl.cpp
    verification-null.cpp)

set(SERVICE_LIB_GEN_SOURCES)

add_gdbus_codegen_with_namespace(SERVICE_LIB_GEN_SOURCES proxy-service com.canonical. proxy ${CMAKE_SOURCE_DIR}/data/com.canonical.pay.xml)
add_gdbus_codegen_with_namespace(SERVICE_LIB_GEN_SOURCES proxy-package com.canonical. proxy ${CMAKE_SOURCE_DIR}/data/com.canonical.pay.package.xml)
add_gdbus_codegen_with_namespace(SERVICE_LIB_GEN_SOURCES proxy-payui com.canonical. proxy ${CMAKE_SOURCE_DIR}/data/com.canonical.pay.payui.xml)

add_library(pay-service-lib STATIC ${SERVICE_LIB_SOURCES} ${SERVICE_LIB_GEN_SOURCES})

message(${SERVICE_DEPS_LIBRARIES})
target_link_libraries(pay-service-lib ${SERVICE_DEPS_LIBRARIES} "-lpthread")

add_executable(pay-service main.cpp)
target_link_libraries(pay-service
                      pay-service-lib)

install(TARGETS pay-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

###########################
# Mir Connection Demangler
###########################

add_executable(mir-connection-demangler mir-connection-demangler.c proxy-payui.c proxy-payui.h)
target_link_libraries(mir-connection-demangler ${SERVICE_DEPS_LIBRARIES})
install(TARGETS mir-connection-demangler RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

#############################
# Untrusted Helper Exec Tool
#############################

add_definitions(-DSOCKET_TOOL="${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}/mir-connection-demangler")
add_executable(exec-tool exec-tool.c)
target_link_libraries(exec-tool ${SERVICE_DEPS_LIBRARIES})
install(TARGETS exec-tool RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ubuntu-app-launch/pay-ui")

######################
# Style Checking
######################

macro(list_prefix _outvar _listvar _prefix)
	set(${_outvar})
	foreach(_item IN LISTS ${_listvar})
		list(APPEND ${_outvar} ${_prefix}${_item})
	endforeach()
endmacro(list_prefix)

list_prefix(pay-service-lib-full-src SERVICE_LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME pay-service-lib-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-lib-full-src})

get_target_property(pay-service-src pay-service SOURCES)
list_prefix(pay-service-full-src pay-service-src "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME pay-service-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-full-src})

