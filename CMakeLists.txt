cmake_minimum_required(VERSION 3.5)

set(PACKAGE_VERSION "0.1")
set(top_srcdir "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_C_FLAGS "-Wall -std=c99")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories("${top_srcdir}")
include_directories("${top_srcdir}/include")

find_package(osl REQUIRED)
find_package(candl REQUIRED)
find_package(clay REQUIRED)

include_directories("${OSL_INCLUDE_DIR}")
include_directories("${CANDL_INCLUDE_DIR}")
include_directories("${CLAY_INCLUDE_DIR}")

file(GLOB src_files "*.c")
file(GLOB include_files "include/*.h")
file(GLOB header_files "*.h")

set(library_files ${src_files})
list(REMOVE_ITEM library_files "${top_srcdir}/main.c")

add_library(clope_obj
            OBJECT
            ${library_files} ${include_files} ${header_files})

add_executable(clope
               "main.c"
               $<TARGET_OBJECTS:clope_obj>)
target_link_libraries(clope ${OSL_LIBRARY})
target_link_libraries(clope ${CANDL_LIBRARY})
target_link_libraries(clope ${CLAY_LIBRARY})

add_library(libclope_so
            SHARED
            $<TARGET_OBJECTS:clope_obj>)
set_target_properties(libclope_so PROPERTIES OUTPUT_NAME clope)
target_link_libraries(libclope_so ${OSL_LIBRARY})
target_link_libraries(libclope_so ${CANDL_LIBRARY})
target_link_libraries(libclope_so ${CLAY_LIBRARY})

add_library(libclope_a
            STATIC
            $<TARGET_OBJECTS:clope_obj>)
set_target_properties(libclope_a PROPERTIES OUTPUT_NAME clope)
target_link_libraries(libclope_a ${OSL_LIBRARY})
target_link_libraries(libclope_a ${CANDL_LIBRARY})
target_link_libraries(libclope_a ${CLAY_LIBRARY})

install(TARGETS clope RUNTIME DESTINATION bin)
install(TARGETS libclope_so LIBRARY DESTINATION lib)
install(TARGETS libclope_a ARCHIVE DESTINATION lib)
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(FILES clope-config.cmake DESTINATION lib/clope)

