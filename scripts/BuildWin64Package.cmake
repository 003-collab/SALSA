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
find_program(CTEST_CMAKE_COMMAND NAMES cmake) # beware: this has spaces in it
find_program(BAT_WIX_COMMAND NAMES GenerateWIXPackage.bat PATHS "${LSA_ROOT}/scripts")

set(CTEST_SOURCE_DIRECTORY "${LSA_ROOT}")
set(CTEST_BINARY_DIRECTORY "${LSA_ROOT}/build")
set(CTEST_LIB_DIRECTORY    "${LSA_ROOT}/lib")
set(CTEST_BUILD_NAME "windows64-msvc - $ENV{CI_BUILD_REF_NAME}")
set(CTEST_CMAKE_GENERATOR "Visual Studio 16 2019")
set(CTEST_CONFIGURATION_TYPE "Release")
set(CTEST_SITE ${LOCAL_HOSTNAME})

set(SALSA_CMAKE_OPTIONS "")
if(SALSA_MP)
    set(SALSA_CMAKE_OPTIONS "-DSALSA_MP=TRUE")
endif()

set(CTEST_CONFIGURE_COMMAND "\"${CTEST_CMAKE_COMMAND}\" \"-G${CTEST_CMAKE_GENERATOR}\" -Wno-dev ${LSA_ROOT} ${SALSA_CMAKE_OPTIONS} ")
set(CTEST_BUILD_COMMAND "\"${CTEST_CMAKE_COMMAND}\" --build . --config release")

ctest_start("Release")

ctest_configure()

if( NOT DEFINED ENV{CTEST_TEST_ONLY} )
    message(STATUS "About to execute build.")
    ctest_build()
endif()

if( DEFINED ENV{LSA_MSI_TARGET} )
    message(STATUS "Launching GenerateWixPackage script...")
    message(STATUS "BAT_WIX_COMMAND = ${BAT_WIX_COMMAND}")
    message(STATUS "Logging wix output to wix_log.txt")

    # CI_COMMIT_REF_NAME won't be set when generating an msi locally
    if( DEFINED ENV{CI_COMMIT_REF_NAME})
        set(BRANCH_NAME $ENV{CI_COMMIT_REF_NAME})
    else()
        set(BRANCH_NAME "local")
    endif()
    
    execute_process(COMMAND ${BAT_WIX_COMMAND} ${CPACK_PACKAGE_NAME} ${BRANCH_NAME} ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
    
endif()

