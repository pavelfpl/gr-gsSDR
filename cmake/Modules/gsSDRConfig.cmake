INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_GSSDR gsSDR)

FIND_PATH(
    GSSDR_INCLUDE_DIRS
    NAMES gsSDR/api.h
    HINTS $ENV{GSSDR_DIR}/include
        ${PC_GSSDR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GSSDR_LIBRARIES
    NAMES gnuradio-gsSDR
    HINTS $ENV{GSSDR_DIR}/lib
        ${PC_GSSDR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gsSDRTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GSSDR DEFAULT_MSG GSSDR_LIBRARIES GSSDR_INCLUDE_DIRS)
MARK_AS_ADVANCED(GSSDR_LIBRARIES GSSDR_INCLUDE_DIRS)
