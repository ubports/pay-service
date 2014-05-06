
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

add_library(pay-service-lib STATIC
            dbus-interface.cpp
            item-interface.h
            item-null.h
            item-memory.h
            item-memory.cpp)

message(${SERVICE_DEPS_LIBRARIES})
target_link_libraries(pay-service-lib ${SERVICE_DEPS_LIBRARIES})

add_executable(pay-service main.cpp)
target_link_libraries(pay-service
                      pay-service-lib
                      ${PROCESS_CPP_LIBRARIES})

install(TARGETS pay-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})

get_target_property(pay-service-lib-src pay-service-lib SOURCES)
add_test(NAME pay-service-lib-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-lib-src})

get_target_property(pay-service-src pay-service SOURCES)
add_test(NAME pay-service-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${pay-service-src})

