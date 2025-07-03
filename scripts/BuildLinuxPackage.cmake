set(LSA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include("${LSA_ROOT}/SalsaConfig.in")

if( DEFINED ENV{LSA_BUILD_TYPE} )
    string( TOUPPER $ENV{LSA_BUILD_TYPE} internal_build_flag)
    if(internal_build_flag STREQUAL "ARL")
        include("${LSA_ROOT}/SalsaConfig_InternalBuilds.in")
    endif()
endif()

set(DEBUG_VALUE "true")

find_program(CTEST_GIT_COMMAND NAMES git)
find_program(CTEST_CMAKE_COMMAND NAMES cmake)
find_program(CTEST_SH_COMMAND NAMES sh)

set(CTEST_SOURCE_DIRECTORY "${LSA_ROOT}")
set(CTEST_BINARY_DIRECTORY "${LSA_ROOT}/build")
set(CTEST_LIB_DIRECTORY    "${LSA_ROOT}/lib")
set(CTEST_CONFIGURATION_TYPE "Release")
cmake_host_system_information(RESULT _host_name QUERY HOSTNAME)
set(CTEST_SITE "${_host_name}")

set(SALSA_CMAKE_OPTIONS "")
if(SALSA_MP)
    set(SALSA_CMAKE_OPTIONS "-DSALSA_MP=TRUE")
endif()

set(CTEST_CONFIGURE_COMMAND "${CTEST_CMAKE_COMMAND} ${LSA_ROOT} ${SALSA_CMAKE_OPTIONS}")
set(CTEST_BUILD_COMMAND "make -j4")
set(CTEST_BUILD_NAME "Ubuntu 20.04 - $ENV{CI_BUILD_REF_NAME}")

#set(CTEST_MEMORYCHECK_COMMAND "/usr/bin/valgrind")
#find_program(CTEST_MEMORYCHECK_COMMAND valgrind)

# Will need to change this when I reimplement valgrind 
#set(CTEST_MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --default-suppressions=yes --error-exitcode=1 --gen-suppressions=all --suppressions=${LSA_ROOT}/suppression.supp")


if( NOT DEFINED ENV{CTEST_TEST_ONLY} )
    # Purge the build directory
    file(REMOVE_RECURSE ${CTEST_BINARY_DIRECTORY})
endif()

# Build
ctest_start("Release")
ctest_configure()

if( NOT DEFINED ENV{CTEST_TEST_ONLY} )
    message(STATUS "About to execute build.")
    ctest_build()
endif()

# Run the memory leak checker
if (DEFINED ENV{MEMCHECK})
    set(RUN_MEMCHECK $ENV{MEMCHECK})
else()
    set(RUN_MEMCHECK "FALSE")
endif()

# Run the memory leak checker
if (RUN_MEMCHECK STREQUAL "TRUE")
    # Run valgrind on the tests
    ctest_memcheck( RETURN_VALUE RC)
    if (NOT RC EQUAL 0)
        # Submit the dashboard results on failure
        #ctest_submit()
        message(FATAL_ERROR "Memcheck failed")
    endif()
endif()

#if( NOT DEFINED ENV{CTEST_TEST_ONLY} )
#    message(STATUS "About to create deb package")
#    #create a .deb file for linux installation
#    execute_process(COMMAND cpack --config ${CTEST_BINARY_DIRECTORY}/CPackConfig.cmake -G DEB )
#endif()


