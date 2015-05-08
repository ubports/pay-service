

set(COMMON_SOURCES
	glib-thread.h)

macro(list_prefix _outvar _listvar _prefix)
	set(${_outvar})
	foreach(_item IN LISTS ${_listvar})
		list(APPEND ${_outvar} ${_prefix}${_item})
	endforeach()
endmacro(list_prefix)

list_prefix(common-full-src COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/")
add_test(NAME common-style COMMAND
	astyle "--options=${CMAKE_SOURCE_DIR}/data/astyle-config" ${common-full-src})
