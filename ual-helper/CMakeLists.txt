
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_WARNING_ARGS} -std=c++11 -g -fPIC ${GCOV_FLAGS}")
include_directories(${CMAKE_CURRENT_BINARY_DIR} "${CMAKE_SOURCE_DIR}/common")


#############################
# Untrusted Helper Exec Tool
#############################

add_executable(exec-tool exec-tool.c)
target_link_libraries(exec-tool ${SERVICE_DEPS_LIBRARIES})
install(TARGETS exec-tool RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/ubuntu-app-launch/pay-ui")
