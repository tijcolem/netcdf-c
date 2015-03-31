SET(libsrcp5_SOURCES ncp5dispatch.c)

add_library(netcdfp5 OBJECT ${libsrcp5_SOURCES})

ADD_EXTRA_DIST(${libsrcp5_SOURCES} CMakeLists.txt)
