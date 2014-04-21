
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

add_library(pay-service-lib STATIC
            dbus-interface.cpp
            item-interface.hpp
            item-null.hpp
            item-memory.hpp
            item-memory.cpp)
target_link_libraries(pay-service-lib ${SERVICE_DEPS_LIBRARIES})

add_executable(pay-service main.cpp)
target_link_libraries(pay-service
                      pay-service-lib)
install(TARGETS pay-service RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_PKGLIBEXECDIR})
