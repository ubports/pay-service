
set(libpay-headers
    pay-package.h)
set(libpay-sources
    pay-package.cpp)

set(api-version 1)
set(abi-version 1)

######################
# Lib Building
######################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

add_library(libpay SHARED ${libpay-sources})
set_target_properties(libpay PROPERTIES
  VERSION ${api-version}.0.0
  SOVESRION ${abi-version}
  OUTPUT_NAME "pay"
)
target_link_libraries(libpay ${SERVICE_DEPS_LIBRARIES})

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
add_test(NAME libpay-headers-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${libpay-headers-full})

list_prefix(libpay-sources-full libpay-sources "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME libpay-sources-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${libpay-sources-full})
