
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

add_library(pay-service-lib STATIC
            dbus-interface.cpp
            item-interface.h
            item-null.h
            item-memory.h
            item-memory.cpp
            purchase-factory.h
            purchase-null.h
            purchase-null.cpp
            purchase-ual.h
            purchase-ual.cpp
            verification-curl.cpp
            verification-null.cpp)

message(${SERVICE_DEPS_LIBRARIES})
target_link_libraries(pay-service-lib ${SERVICE_DEPS_LIBRARIES})

add_executable(pay-service main.cpp)
target_link_libraries(pay-service
                      pay-service-lib
                      ${PROCESS_CPP_LIBRARIES})

install(TARGETS pay-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

######################
# Style Checking
######################

macro(list_prefix _outvar _listvar _prefix)
	set(${_outvar})
	foreach(_item IN LISTS ${_listvar})
		list(APPEND ${_outvar} ${_prefix}${_item})
	endforeach()
endmacro(list_prefix)

get_target_property(pay-service-lib-src pay-service-lib SOURCES)
list_prefix(pay-service-lib-full-src pay-service-lib-src "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME pay-service-lib-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-lib-full-src})

get_target_property(pay-service-src pay-service SOURCES)
list_prefix(pay-service-full-src pay-service-src "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME pay-service-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-full-src})

