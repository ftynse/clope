# Try to find the clope library

# CLOPE_FOUND       - System has clope lib
# CLOPE_INCLUDE_DIR - The clope include directory
# CLOPE_LIBRARY     - Library needed to use clope

if (CLOPE_INCLUDE_DIR AND CLOPE_LIBRARY)
        # Already in cache, be silent
        set(CLOPE_FIND_QUIETLY TRUE)
endif()

find_path(CLOPE_INCLUDE_DIR NAMES clope/clope.h)
find_library(CLOPE_LIBRARY NAMES clope)

if (CLOPE_LIBRARY AND CLOPE_INCLUDE_DIR)
        message(STATUS "Library clope found =) ${CLOPE_LIBRARY}")
else()
        message(STATUS "Library clope not found =(")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CLOPE DEFAULT_MSG CLOPE_INCLUDE_DIR CLOPE_LIBRARY)

mark_as_advanced(CLOPE_INCLUDE_DIR CLOPE_LIBRARY)
