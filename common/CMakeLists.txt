

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -fPIC ${GCOV_FLAGS}")

set(COMMON_SOURCES
    glib-thread.cpp
    glib-thread.h)

add_library(common-lib STATIC ${COMMON_SOURCES})

macro(list_prefix _outvar _listvar _prefix)
    set(${_outvar})
    foreach(_item IN LISTS ${_listvar})
        list(APPEND ${_outvar} ${_prefix}${_item})
    endforeach()
endmacro(list_prefix)

list_prefix(common-full-src COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/")
