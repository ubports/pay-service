
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

add_library(pay-service-lib STATIC
            dbus-interface.cpp
            item-interface.h
            item-null.h
            item-memory.h
            item-memory.cpp
            verification-curl.cpp
            verification-null.cpp)

message(${SERVICE_DEPS_LIBRARIES})
target_link_libraries(pay-service-lib ${SERVICE_DEPS_LIBRARIES})

add_executable(pay-service main.cpp)
target_link_libraries(pay-service
                      pay-service-lib
                      ${PROCESS_CPP_LIBRARIES})

install(TARGETS pay-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})
