
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g ${GCOV_FLAGS}")

set(libpay-headers
    pay-package.h)


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
