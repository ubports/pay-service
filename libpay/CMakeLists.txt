
set(libpay-headers
    pay-package.h)
set(libpay-sources
    pay-package.cpp)

set(api-version 2)
set(abi-version 2)

######################
# DBus Proxies
######################

set(libpay-generated)
add_gdbus_codegen_with_namespace(libpay-generated proxy-package com.canonical. proxy ${CMAKE_SOURCE_DIR}/data/com.canonical.pay.package.xml)

######################
# Lib Building
######################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS} -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g ${GCOV_FLAGS} -fvisibility=hidden")
include_directories(SYSTEM ${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR})

add_library(libpay SHARED ${libpay-sources} ${libpay-generated})
set_target_properties(libpay PROPERTIES
  VERSION ${api-version}.0.0
  SOVESRION ${abi-version}
  OUTPUT_NAME "pay"
)

target_link_libraries(libpay ${SERVICE_DEPS_LIBRARIES} common-lib)
set_target_properties(libpay PROPERTIES LINK_FLAGS "${ldflags} -Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/libpay.map ")
set_target_properties(libpay PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libpay.map")

######################
# Package Config
######################

configure_file("pay.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/pay-${api-version}.pc" @ONLY)

######################
# Style Checking
######################

macro(list_prefix _outvar _listvar _prefix)
    set(${_outvar})
    foreach(_item IN LISTS ${_listvar})
        list(APPEND ${_outvar} ${_prefix}${_item})
    endforeach()
endmacro(list_prefix)

list_prefix(libpay-headers-full libpay-headers "${CMAKE_CURRENT_SOURCE_DIR}/")
list_prefix(libpay-sources-full libpay-sources "${CMAKE_CURRENT_SOURCE_DIR}/")

######################
# Installation
######################

install(
  TARGETS libpay
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  FILES ${libpay-headers}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libpay-${api-version}/libpay"
)

install (
  FILES "${CMAKE_CURRENT_BINARY_DIR}/pay-${api-version}.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
