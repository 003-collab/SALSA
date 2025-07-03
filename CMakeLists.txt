cmake_minimum_required(VERSION 3.18.4)

#----------------------------------------
# Don't allow in-source builds.
#----------------------------------------
 if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build in lsa/build.  See lsa/install.txt for more installation details.")
  endif()

# Create the project
project(lsa)

#----------------------------------------
# CMAKE_INSTALL_PREFIX
#
# Determine Installation Path Prefix
#     Path to which we will install the
#     executables.
#----------------------------------------

if (DEFINED ENV{LSA_BUILD_TYPE})
    string( TOUPPER $ENV{LSA_BUILD_TYPE} internal_build_flag)
    if(internal_build_flag STREQUAL "ARL")
        set(IS_ARL_BUILD True)
    endif()
endif()
message( STATUS "IS_ARL_BUILD: ${IS_ARL_BUILD}" )

if( (DEFINED CMAKE_INSTALL_PREFIX) AND (NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT))
    message( STATUS "CMAKE_INSTALL_PREFIX was defined by user with the cmake command line. CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
elseif( DEFINED ENV{lsasolver} )
    set( CMAKE_INSTALL_PREFIX $ENV{lsasolver}  CACHE PATH "default install path" FORCE)
    message( STATUS "CMAKE_INSTALL_PREFIX was defined the environment variable ENV{lsasolver}. CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX} " )
else()
    set (CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install" CACHE PATH "default install path" FORCE)
    message( STATUS "Using default CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}.
    Use -DCMAKE_INSTALL_PREFIX or set ENV{lsasolver} to change." )
endif()

#----------------------------------------
# Option Switches
#
# Usage: option( <option_variable> "help string" [initial value] )
#----------------------------------------

option( DEBUG_SWITCH "HELP: DEBUG_SWITCH: Default = OFF, print some CMake variable values to stdout." OFF )
option( DEBUG_VERBOSE "HELP: DEBUG_VERBOSE: Default = OFF, print all CMake variable values." OFF )
option( TEST_SWITCH "HELP: TEST_SWITCH: SWITCH, Default = ON, Turn on test mode." ON )

#----------------------------------------
# Find Python Interpretor
#----------------------------------------
if( WIN32 )
    set(PYTHON_EXE "${CMAKE_CURRENT_LIST_DIR}/python/python.exe")
    set(SALSA_PYTHON_DIR "${CMAKE_CURRENT_LIST_DIR}/python/Library")
else()
    set(PYTHON_EXE $ENV{SALSA_PYTHON_EXE})
    get_filename_component(SALSA_PYTHON_DIR ${PYTHON_EXE} DIRECTORY)
endif()
message( STATUS "PYTHON_EXE was defined as ${PYTHON_EXE}" )



#----------------------------------------
# Debug Messages
#
# The CMake command-line tool, message( STATUS ) displays
# messages on stdout and all other message types on stderr.
#----------------------------------------

if( DEBUG_SWITCH )
    message( STATUS "DEBUG: CMAKE_SYSTEM             = ${CMAKE_SYSTEM}" )      # e.g., Linux-3.2.0
    message( STATUS "DEBUG: CMAKE_SYSTEM_NAME        = ${CMAKE_SYSTEM_NAME}" ) # e.g., Linux
    message( STATUS "DEBUG: CMAKE_COMMAND            = ${CMAKE_COMMAND}" )     # e.g., /usr/bin/cmake
    message( STATUS "DEBUG: CMAKE_VERSION            = ${CMAKE_VERSION}" )     # e.g., 2.8.9
    message( STATUS "DEBUG: CMAKE_BUILD_TOOL         = ${CMAKE_BUILD_TOOL}" )  # e.g., /usr/bin/make
    message( STATUS "DEBUG: CMAKE_BUILD_TYPE         = ${CMAKE_BUILD_TYPE}" )  # e.g., release, debug, etc.
    message( STATUS "DEBUG: PYTHON_EXE               = ${PYTHON_EXE}" )        # e.g., ~/.local/salsa_python
    message( STATUS "DEBUG: CMAKE_CURRENT_LIST_DIR   = ${CMAKE_CURRENT_LIST_DIR}" )    # e.g., $HOME/git/lsa
    message( STATUS "DEBUG: CMAKE_CURRENT_LIST_FILE  = ${CMAKE_CURRENT_LIST_FILE}" )   # e.g., $HOME/git/lsa/CMakeLists.txt
    message( STATUS "DEBUG: CMAKE_SOURCE_DIR         = ${CMAKE_SOURCE_DIR}" )          # e.g., $HOME/git/lsa
    message( STATUS "DEBUG: CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}" )  # e.g., $HOME/git/lsa
    message( STATUS "DEBUG: CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}" )  # e.g., $HOME/git/lsa/build
    message( STATUS "DEBUG: PROJECT_NAME             = ${PROJECT_NAME}" )              # e.g., lsa
    message( STATUS "DEBUG: PROJECT_BINARY_DIR       = ${PROJECT_BINARY_DIR}" )        # e.g., $HOME/git/lsa/build
    message( STATUS "DEBUG: PROJECT_SOURCE_DIR       = ${PROJECT_SOURCE_DIR}" )        # e.g., $HOME/git/lsa/src
    message( STATUS "DEBUG: CMAKE_INSTALL_DIR        = ${CMAKE_INSTALL_DIR}" )         # e.g., /usr
    message( STATUS "DEBUG: CMAKE_INSTALL_PREFIX     = ${CMAKE_INSTALL_PREFIX}" )      # e.g., $HOME/lsa
    message( STATUS "DEBUG: CMAKE_BINARY_DIR         = ${CMAKE_BINARY_DIR}" )          # e.g., $HOME/lsa

endif()


#----------------------------------------
# Update Salsa version file
#----------------------------------------

if(IS_ARL_BUILD)
    execute_process(COMMAND ${PYTHON_EXE} ${CMAKE_CURRENT_LIST_DIR}/scripts/build/versions.py WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
endif()

#----------------------------------------
# Set Platform-Dependent Compiler options
#----------------------------------------

# is this a debug build?
if(CMAKE_BUILD_TYPE)
    string(TOUPPER ${CMAKE_BUILD_TYPE} upperBuildType)
    string(COMPARE EQUAL ${upperBuildType} "DEBUG" isDebugBuild)
else()
    set(isDebugBuild 0)
endif()

if( UNIX )
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -shared" )
    add_compile_options(-std=c++14 -Wno-deprecated -Wno-deprecated-declarations)

    # Don't enable O3 optimizations for linux debug builds
    if(isDebugBuild)
        message(STATUS "Debug build: O3 optimizations not enabled.")
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -fopenmp" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fopenmp" )
    endif()
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
elseif( WIN32 )
    add_definitions(/DNOMINMAX /D_NO_DEPRECATED /D_NO_DEPRECATED_DECLARATIONS /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES /wd"4274" /wd"4503" /wd"4290" /wd"4267" /wd"4250" /wd"4244" /wd"4101" /wd"4800" /wd"4068" )
    add_compile_options(/GR)
    add_compile_options(/openmp)

    if("${SALSA_MP}" STREQUAL "TRUE")
            add_compile_options( /MP )
    endif()

endif( UNIX )

#----------------------------------------
# Debug Verbose: print all cmake variables
#----------------------------------------

if( DEBUG_VERBOSE )
        get_cmake_property( _variableNames VARIABLES )
    foreach( _variableName ${_variableNames} )
        message( STATUS "---- DEBUG VERBOSE: ${_variableName} = ${${_variableName}}" )
        endforeach()
endif()

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

#----------------------------------------
#
# Use the same directory structure for windows builds as linux builds
# (Always build in lsa/build without lsa/build/Release or lsa/build/Debug)
#
#----------------------------------------
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_BINARY_DIR}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_BINARY_DIR}")

#----------------------------------------
# Set the location for Qt5
#----------------------------------------
# TODO: remove hardcoded paths to make this less of a hack
if( DEFINED ENV{CMAKE_PREFIX_PATH} )
    # if the environment var is set, we don't have to do anything
else()
  if( UNIX )
     set( CMAKE_PREFIX_PATH "~/Qt5.15.8" )
  endif()
endif()

#----------------------------------------
# Define source files
#----------------------------------------

# lsacommon files
file( GLOB_RECURSE LSACOMMON_HDR "src/lsacommon/*.hpp" )
file( GLOB_RECURSE LSACOMMON_SRC "src/lsacommon/*.cpp" )

## lsacwrapper files
file( GLOB_RECURSE LSAWRAPPERS_HDR "src/lsawrappers/*.hpp" )
file( GLOB_RECURSE LSAWRAPPERS_SRC "src/lsawrappers/*.cpp" )

# lsasolver files
file( GLOB_RECURSE SOLVER_HDR "src/lsasolver/*.hpp" )
file( GLOB_RECURSE SOLVER_SRC "src/lsasolver/*.cpp" )
file( GLOB_RECURSE POST_HDR "src/lsasolver/*.hpp" )
file( GLOB_RECURSE POST_SRC "src/lsasolver/*.cpp" )
file( GLOB_RECURSE INVERSE_HDR "src/lsasolver/*.hpp" )
file( GLOB_RECURSE INVERSE_SRC "src/lsasolver/*.cpp" )

# lsapreprocessor files
file( GLOB_RECURSE PREPROC_HDR "src/lsapreprocessor/*.hpp" )
file( GLOB_RECURSE PREPROC_SRC "src/lsapreprocessor/*.cpp" )

# iob converter files
file( GLOB_RECURSE CONVERT_HDR "src/converters/iob/*.hpp" )
file( GLOB_RECURSE CONVERT_SRC "src/converters/iob/*.cpp" )

# gui files
file( GLOB_RECURSE GUI_HDR "src/gui/*.hpp" )
file( GLOB_RECURSE GUI_SRC "src/gui/*.cpp" )
file( GLOB_RECURSE GUI_UI  "src/gui/*.ui" )

# Specify the windows desktop icon for the salsa.exe gui
if( WIN32 )
    set (GUI_RC_FILE "salsa.rc")
endif()

# geoid files
file( GLOB_RECURSE GEOID_HDR "src/geoid/*.hpp" )
file( GLOB_RECURSE GEOID_SRC "src/geoid/*.cpp" )

#----------------------------------------
# Define include directories
#----------------------------------------

# initialize list of include directories
set( INCLUDE_DIRS "" )

foreach( _headerFile ${LSACOMMON_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${LSAWRAPPERS_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${SOLVER_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${PREPROC_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${CONVERT_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${GUI_HDR} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

foreach( _headerFile ${GUITEST_SRC} )
    get_filename_component( _dir ${_headerFile} PATH )
    list( APPEND INCLUDE_DIRS ${_dir} )
endforeach()

list( REMOVE_DUPLICATES INCLUDE_DIRS )

# Add every directory containing a header file
# to the project include_directories
include_directories( ${INCLUDE_DIRS} )

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#----------------------------------------
# Find HDF5
#----------------------------------------

if(WIN32)
    set(HDF5_ROOT "${SALSA_PYTHON_DIR}")
    find_package(HDF5 1.14.3 COMPONENTS C CXX HL)
    add_definitions(/DH5_BUILT_AS_DYNAMIC_LIB)
else()
    set(HDF5_ROOT "${SALSA_PYTHON_DIR}/../")
    find_package(HDF5 1.14.3 COMPONENTS C CXX HL )
endif()

if (HDF5_FOUND)
  message(STATUS "Found HDF5 at ${HDF5_INCLUDE_DIRS}")
  message(STATUS "HDF5 libs: ${HDF5_CXX_LIBRARIES}")
  include_directories( ${HDF5_INCLUDE_DIRS} )
endif(HDF5_FOUND)


#----------------------------------------
# Find gnsstk library
#----------------------------------------

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules )

find_package( GNSSTK )

if( DEBUG_SWITCH )
    message( STATUS "DEBUG: GNSSTK_INCLUDE_DIRS = ${GNSSTK_INCLUDE_DIRS}" )
    message( STATUS "DEBUG: GNSSTK_LIBRARIES = ${GNSSTK_LIBRARIES}" )
    message( STATUS "DEBUG: GNSSTK_INCLUDE_DIRS_DEBUG = ${GNSSTK_INCLUDE_DIRS_DEBUG}" )
    message( STATUS "DEBUG: GNSSTK_LIBRARIES_DEBUG = ${GNSSTK_LIBRARIES_DEBUG}" )
endif()

include_directories( ${GNSSTK_INCLUDE_DIRS})

if(EXISTS ${GNSSTK_INCLUDE_DIRS_DEBUG})
    include_directories( ${GNSSTK_INCLUDE_DIRS_DEBUG})
endif()

#----------------------------------------
# Find MARBLE library
#----------------------------------------

find_package( MARBLE )

include_directories( ${MARBLE_INCLUDE_DIRS} )
if(EXISTS ${MARBLE_INCLUDE_DIRS_DEBUG})
    include_directories( ${MARBLE_INCLUDE_DIRS_DEBUG})
endif()

if( DEBUG_SWITCH )
    message( STATUS "DEBUG: MARBLE_INCLUDE_DIRS_DEBUG = ${MARBLE_INCLUDE_DIRS_DEBUG}" )
    message( STATUS "DEBUG: MARBLE_LIBRARIES_DEBUG = ${MARBLE_LIBRARIES_DEBUG}" )
    message( STATUS "DEBUG: MARBLE_INCLUDE_DIRS_DEBUG = ${MARBLE_INCLUDE_DIRS_DEBUG}" )
    message( STATUS "DEBUG: MARBLE_LIBRARIES_DEBUG = ${MARBLE_LIBRARIES_DEBUG}" )
endif()

#----------------------------------------
# Include Eigen3 library
#----------------------------------------
find_package( EIGEN3 )
include_directories( ${EIGEN3_INCLUDE_DIRS} )

#----------------------------------------
# Find Qwt library
#----------------------------------------

find_package( QWT )
include_directories( ${QWT_INCLUDE_DIRS} )

#----------------------------------------
# Find some Qt libraries
#----------------------------------------
# Make sure the Qt install location is captured as the CMAKE_PREFIX_PATH environment variable
if( DEFINED ENV{CMAKE_PREFIX_PATH} )
    message( STATUS "Qt install location set to $ENV{CMAKE_PREFIX_PATH}")
else()
    message( STATUS "")
    message( FATAL_ERROR "ERROR: Qt install location must be defined in CMAKE_PREFIX_PATH environment variable")
    message( STATUS "")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Test REQUIRED)

#----------------------------------------
# Find zlib
#----------------------------------------
if( WIN32 )
    set(ZLIB_DLL_PATH "${SALSA_PYTHON_DIR}/bin")
    set(ZLIB_REQUISITE_DLLS zlib.dll)
endif()

#----------------------------------------
# Find libcrypto-3
#----------------------------------------
if( WIN32 )
    set(LIBCRYPTO_DLL_PATH "${SALSA_PYTHON_DIR}/bin")
    set(LIBCRYPTO_REQUISITE_DLLS libcrypto-3-x64.dll)
endif()

#----------------------------------------
# Find libcurl
#----------------------------------------
if( WIN32 )
    set(LIBCURL_DLL_PATH "${SALSA_PYTHON_DIR}/bin")
    set(LIBCURL_REQUISITE_DLLS libcurl.dll)
endif()

#----------------------------------------
# Find szip
#----------------------------------------
if( WIN32 )
    set(SZIP_DLL_PATH "${SALSA_PYTHON_DIR}/bin")
    set(SZIP_REQUISITE_DLLS szip.dll)
endif()

#----------------------------------------
# Find libssh2
#----------------------------------------
if( WIN32 )
    set(LIBSSH_DLL_PATH "${SALSA_PYTHON_DIR}/bin")
    set(LIBSSH_REQUISITE_DLLS libssh2.dll)
endif()

#----------------------------------------
# Find QuaZip library
#----------------------------------------
if( WIN32 )
    if( isDebugBuild )
        FIND_LIBRARY(QUAZIP_LIBRARIES
                NAMES quazip quazip5 quazip1-qt5d lcompress
                HINTS "C:/QuaZip_debug/lib"
                REQUIRED
        )
        FIND_PATH(QUAZIP_INCLUDE_DIRS quazip
                HINTS "C:/QuaZip_debug/include/QuaZip-Qt5-1.4"
                PATH_SUFFIXES quazip
                REQUIRED
        )
        FIND_PATH(QUAZIP_LIBRARIES_DIR quazip1-qt5d.dll HINTS "C:/QuaZip_debug/bin" REQUIRED)
    else()
        FIND_LIBRARY(QUAZIP_LIBRARIES
                NAMES quazip quazip5 quazip1-qt5 lcompress
                HINTS "C:/QuaZip/lib"
                REQUIRED
        )
        FIND_PATH(QUAZIP_INCLUDE_DIRS quazip
                HINTS "C:/QuaZip/include/QuaZip-Qt5-1.4"
                PATH_SUFFIXES quazip
                REQUIRED
        )
        FIND_PATH(QUAZIP_LIBRARIES_DIR quazip1-qt5.dll HINTS "C:/QuaZip/bin" REQUIRED)
    endif()

else()
    FIND_LIBRARY(QUAZIP_LIBRARIES
            NAMES quazip quazip5 quazip-qt5 jlcompress
            HINTS /usr/lib /usr/lib64
            REQUIRED
    )
    FIND_PATH(QUAZIP_INCLUDE_DIRS quazip.h
            HINTS /usr/include /usr/local/include
            PATH_SUFFIXES quazip
            REQUIRED
    )
endif()

include_directories( ${QUAZIP_INCLUDE_DIRS} )
message( STATUS "QUAZIP_INCLUDE_DIRS = ${QUAZIP_INCLUDE_DIRS}" )
message( STATUS "QUAZIP_LIBRARIES = ${QUAZIP_LIBRARIES}" )
message( STATUS "QUAZIP_LIBRARIES_DIR = ${QUAZIP_LIBRARIES_DIR}" )

#----------------------------------------
# Define the lsacommon library
#----------------------------------------
SET (LSACOMMON_NAME lsacommon)
add_library(${LSACOMMON_NAME} ${LSACOMMON_HDR} ${LSACOMMON_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${LSACOMMON_NAME} Qt5::Core optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${LSACOMMON_NAME} Qt5::Core ${GNSSTK_LIBRARIES})
endif()

target_link_libraries(${LSACOMMON_NAME} ${HDF5_LIBRARIES})

# do not install lsacommon
# install( TARGETS ${LSACOMMON_NAME}
#          ARCHIVE DESTINATION lib
#          LIBRARY DESTINATION lib
#          COMPONENT library )

#----------------------------------------
# Define the lsawrappers library
#----------------------------------------
SET (LSAWRAPPERS_NAME lsawrappers)
add_library(${LSAWRAPPERS_NAME} SHARED ${LSAWRAPPERS_HDR} ${LSAWRAPPERS_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${LSAWRAPPERS_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${LSAWRAPPERS_NAME} ${GNSSTK_LIBRARIES})
endif()

target_link_libraries(${LSAWRAPPERS_NAME} ${LSACOMMON_NAME})

if(UNIX)
    set(LSAWRAPPERS_LIB_NAME $<TARGET_SONAME_FILE_NAME:${LSAWRAPPERS_NAME}>)
else()
    set(LSAWRAPPERS_LIB_NAME "$<TARGET_NAME:${LSAWRAPPERS_NAME}>.dll")
endif(UNIX)

add_custom_command(TARGET ${LSAWRAPPERS_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${LSAWRAPPERS_NAME}>
        ${CMAKE_CURRENT_LIST_DIR}/lib/${LSAWRAPPERS_LIB_NAME})

# install executables and lsawrapper:
install( TARGETS ${LSAWRAPPERS_NAME}
       # ARCHIVE DESTINATION lib ## static libraries
         RUNTIME DESTINATION lib ## DLLs and EXEs
         LIBRARY DESTINATION lib ) ## non-dll shared libraries

#----------------------------------------
# Define the lsasolver executable
#----------------------------------------
SET (SOLVER_EXE_NAME lsasolver)
list(REMOVE_ITEM SOLVER_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsapost.cpp" )
list(REMOVE_ITEM SOLVER_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsainverse.cpp" )

add_executable(${SOLVER_EXE_NAME} ${SOLVER_HDR} ${SOLVER_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${SOLVER_EXE_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${SOLVER_EXE_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${SOLVER_EXE_NAME} ${LSACOMMON_NAME})

# install lsasolver:
install( TARGETS ${SOLVER_EXE_NAME} DESTINATION bin )

#----------------------------------------
# Define the lsapost executable
#----------------------------------------
SET (POST_EXE_NAME lsapost)
list(REMOVE_ITEM POST_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsaeigen.cpp" )
list(REMOVE_ITEM POST_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsasolver.cpp" )
list(REMOVE_ITEM POST_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsacmdline.cpp" )
list(REMOVE_ITEM POST_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsapriori.cpp" )
list(REMOVE_ITEM POST_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsainverse.cpp" )
add_executable(${POST_EXE_NAME} ${POST_HDR} ${POST_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${POST_EXE_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${POST_EXE_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${POST_EXE_NAME} ${LSACOMMON_NAME})

# install lsapost:
install( TARGETS ${POST_EXE_NAME} DESTINATION bin )

#----------------------------------------
# Define the lsainverse executable
#----------------------------------------
SET (INVERSE_EXE_NAME lsainverse)
list(REMOVE_ITEM INVERSE_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsaeigen.cpp" )
list(REMOVE_ITEM INVERSE_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsasolver.cpp" )
list(REMOVE_ITEM INVERSE_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsacmdline.cpp" )
list(REMOVE_ITEM INVERSE_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsapriori.cpp" )
list(REMOVE_ITEM INVERSE_SRC "${CMAKE_CURRENT_LIST_DIR}/src/lsasolver/lsapost.cpp" )
add_executable(${INVERSE_EXE_NAME} ${INVERSE_HDR} ${INVERSE_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${INVERSE_EXE_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${INVERSE_EXE_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${INVERSE_EXE_NAME} ${LSACOMMON_NAME})

# install lsainverse:
install( TARGETS ${INVERSE_EXE_NAME} DESTINATION bin )

#----------------------------------------
# Define the preprocessor executable
#----------------------------------------
SET (PREPROC_EXE_NAME lsapreprocessor)

add_executable(${PREPROC_EXE_NAME} ${PREPROC_HDR} ${PREPROC_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${PREPROC_EXE_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${PREPROC_EXE_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${PREPROC_EXE_NAME} ${LSACOMMON_NAME})

# install preprocessor:
install( TARGETS ${PREPROC_EXE_NAME} DESTINATION bin )

#----------------------------------------
# Define the converter executable
#----------------------------------------
SET (CONVERT_EXE_NAME iob2lsa)
add_executable(${CONVERT_EXE_NAME} ${CONVERT_HDR} ${CONVERT_SRC})

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${CONVERT_EXE_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${CONVERT_EXE_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${CONVERT_EXE_NAME} ${LSACOMMON_NAME})

# install iob2lsa:
install( TARGETS ${CONVERT_EXE_NAME} DESTINATION bin )

#----------------------------------------
# Define the cgeoid executable
#----------------------------------------
SET (CGEOID_NAME cgeoid)
add_executable(${CGEOID_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/geoid/cgeoid.cpp)

# link to the gnsstk library
if(EXISTS ${GNSSTK_LIBRARIES_DEBUG})
    target_link_libraries(${CGEOID_NAME} optimized ${GNSSTK_LIBRARIES} debug ${GNSSTK_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${CGEOID_NAME} ${GNSSTK_LIBRARIES})
endif()

# link to the lsacommon library
target_link_libraries(${CGEOID_NAME} ${LSACOMMON_NAME})

install(TARGETS ${CGEOID_NAME} DESTINATION bin )

##----------------------------------------
## Define the gui executable
##----------------------------------------
SET (GUI_NAME salsa)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

QT5_WRAP_UI(UIS_HDRS ${GUI_UI})

# resources.qrc is an XML file that specifies all of the icons used by
# the mapping widget
set(lsagui_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.qrc)

# add the previously specified image resources
QT5_ADD_RESOURCES(lsagui_RESOURCES_RCC ${lsagui_RESOURCES})

# Tell CMake to create the salsa executable
add_executable(${GUI_NAME} ${GUI_HDR} ${GUI_SRC} ${GUI_UI} ${GUI_RC_FILE} ${lsagui_RESOURCES_RCC})

# Use the Widgets module from Qt 5
target_link_libraries(${GUI_NAME} Qt5::Core Qt5::Gui Qt5::Widgets ${LSACOMMON_NAME} ${QUAZIP_LIBRARIES})

# Link against marble
if(EXISTS ${MARBLE_LIBRARIES_DEBUG})
    target_link_libraries(${GUI_NAME} optimized ${MARBLE_LIBRARIES} debug ${MARBLE_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${GUI_NAME} ${MARBLE_LIBRARIES})
endif()

if(NOT WIN32)
    target_compile_definitions(${GUI_NAME} PUBLIC QT_STRICT_ITERATORS)
endif()

#link against qwt
if(EXISTS ${QWT_LIBRARIES_DEBUG})
    target_link_libraries(${GUI_NAME} optimized ${QWT_LIBRARIES} debug ${QWT_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${GUI_NAME} ${QWT_LIBRARIES})
endif()

# install salsa executable:
install( TARGETS ${GUI_NAME} DESTINATION bin )

#----------------------------------------
# Public GUI Test Harness Executable
#----------------------------------------

# NOTE: the test harness reuses all of the code from the gui except for main.cpp.
# To build the test harness, replicate most of how we build the gui, but we
# substitue our main method that calls all of our test defs for the gui main method.

set (TESTPUBLIC_NAME lsatestpublic)

# Remove src/gui/main.cpp so we don't have two main methods defined
list(REMOVE_ITEM GUI_SRC "${CMAKE_CURRENT_LIST_DIR}/src/gui/main.cpp")

set (PUBLIC_GUI_TEST_PATH src/public_guitest)
add_executable(${TESTPUBLIC_NAME} ${PUBLIC_GUI_TEST_PATH}/testpubliclsagui.cpp ${GUI_HDR} ${GUI_SRC} ${GUI_UI} ${lsagui_RESOURCES_RCC})

# link to the lsacommon library
target_link_libraries(${TESTPUBLIC_NAME} ${LSACOMMON_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Test ${QUAZIP_LIBRARIES})

# Link against marble
if(EXISTS ${MARBLE_LIBRARIES_DEBUG})
    target_link_libraries(${TESTPUBLIC_NAME} optimized ${MARBLE_LIBRARIES} debug ${MARBLE_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${TESTPUBLIC_NAME} ${MARBLE_LIBRARIES})
endif()

#link against qwt
if(EXISTS ${QWT_LIBRARIES_DEBUG})
    target_link_libraries(${TESTPUBLIC_NAME} optimized ${QWT_LIBRARIES} debug ${QWT_LIBRARIES_DEBUG} )
else()
    target_link_libraries(${TESTPUBLIC_NAME} ${QWT_LIBRARIES})
endif()

# do not install

##----------------------------------------
## Define internal build executables
##----------------------------------------
if(IS_ARL_BUILD)
    include(CMakeLists_InternalBuilds.txt)
endif()

#----------------------------------------
# Begin Packaging
#----------------------------------------
include( "${CMAKE_CURRENT_LIST_DIR}/SalsaConfig.in" )
if(IS_ARL_BUILD)
    include( "${CMAKE_CURRENT_LIST_DIR}/SalsaConfig_InternalBuilds.in" )
endif()

#----------------------------------------
# Install contents of data directory
#----------------------------------------
install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data" DESTINATION . )

#----------------------------------------
# Install python, converters
#----------------------------------------
if(WIN32)
    install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/python" DESTINATION . )
endif()
install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/resources/mapIcons" DESTINATION resources )

install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/scripts/converters" DESTINATION scripts )

install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/scripts/geoid" DESTINATION scripts )

install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/scripts/reporting" DESTINATION scripts )

install( DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/scripts/msi" DESTINATION scripts )

#----------------------------------------
# Install Qt DLLs
#----------------------------------------
set( HDF_DLL_PATH "${SALSA_PYTHON_DIR}/bin" )
set( HDF_REQUISITE_DLLS  hdf5_cpp.dll hdf5.dll)

file(TO_CMAKE_PATH "$ENV{CMAKE_PREFIX_PATH}/bin" QT5_REQUISITE_DLL_PATH)

if(isDebugBuild)
    set( QT5_REQUISITE_DLLS
         Qt5Xmld.dll
         Qt5Widgetsd.dll
         Qt5Svgd.dll
         Qt5Sqld.dll
         Qt5Sensorsd.dll
         Qt5Scriptd.dll
         Qt5Testd.dll
         Qt5Quickd.dll
         Qt5Qmld.dll
         Qt5QmlModelsd.dll
         Qt5PrintSupportd.dll
         Qt5Positioningd.dll
         Qt5OpenGLd.dll
         Qt5Networkd.dll
         Qt5MultimediaWidgetsd.dll
         Qt5Multimediad.dll
         Qt5Guid.dll
         Qt5Cored.dll
         libGLESv2d.dll
         libEGLd.dll )
else()
    set( QT5_REQUISITE_DLLS
         Qt5Xml.dll
         Qt5Widgets.dll
         Qt5Svg.dll
         Qt5Sql.dll
         Qt5Sensors.dll
         Qt5Script.dll
         Qt5Test.dll
         Qt5Quick.dll
         Qt5Qml.dll
         Qt5QmlModels.dll
         Qt5PrintSupport.dll
         Qt5Positioning.dll
         Qt5OpenGL.dll
         Qt5Network.dll
         Qt5MultimediaWidgets.dll
         Qt5Multimedia.dll
         Qt5Gui.dll
         Qt5Core.dll
         libGLESv2.dll
         libEGL.dll )
endif()

function( prepend var prefix )
    set(listVar "")
    foreach(f ${ARGN})
        list(APPEND listVar "${prefix}${f}")
    endforeach(f)
    set(${var} "${listVar}" PARENT_SCOPE)
endfunction( prepend )

function( listappend var postfix )
    set(listVar "")
    foreach(f ${ARGN})
        list(APPEND listVar "${f}${postfix}")
    endforeach(f)
    set(${var} "${listVar}" PARENT_SCOPE)
endfunction( listappend )

if(WIN32)
    prepend( QT5_FILES_TO_COPY ${QT5_REQUISITE_DLL_PATH}/ ${QT5_REQUISITE_DLLS} )
    prepend( HDF5_FILES_TO_COPY ${HDF_DLL_PATH}/ ${HDF_REQUISITE_DLLS} )
    prepend( ZLIB_FILES_TO_COPY ${ZLIB_DLL_PATH}/ ${ZLIB_REQUISITE_DLLS} )
    prepend( LIBCRYPTO_FILES_TO_COPY ${LIBCRYPTO_DLL_PATH}/ ${LIBCRYPTO_REQUISITE_DLLS} )
    prepend( LIBCURL_FILES_TO_COPY ${LIBCURL_DLL_PATH}/ ${LIBCURL_REQUISITE_DLLS} )
    prepend( SZIP_FILES_TO_COPY ${SZIP_DLL_PATH}/ ${SZIP_REQUISITE_DLLS} )
    prepend( LIBSSH_FILES_TO_COPY ${LIBSSH_DLL_PATH}/ ${LIBSSH_REQUISITE_DLLS} )

    install( FILES ${QT5_FILES_TO_COPY}
             DESTINATION bin )

    install( FILES ${ZLIB_FILES_TO_COPY}
             DESTINATION bin )

    if(isDebugBuild)
        install( FILES ${QT5_REQUISITE_DLL_PATH}/../plugins/platforms/qwindowsd.dll
                  DESTINATION bin/platforms/ )
    else()
        install( FILES ${QT5_REQUISITE_DLL_PATH}/../plugins/platforms/qwindows.dll
                 DESTINATION bin/platforms/ )
    endif()

    install( FILES ${HDF5_FILES_TO_COPY} DESTINATION bin )

    install ( FILES ${QWT_LIBRARY_DIRS}/qwt.dll DESTINATION bin )

    install ( FILES ${QUAZIP_LIBRARIES_DIR}/quazip1-qt5.dll DESTINATION bin )

    install ( FILES ${LIBCRYPTO_FILES_TO_COPY} DESTINATION bin )

    install ( FILES ${LIBCURL_FILES_TO_COPY} DESTINATION bin )

    install ( FILES ${SZIP_FILES_TO_COPY} DESTINATION bin )

    install (FILES ${LIBSSH_FILES_TO_COPY} DESTINATION bin )

    # copy dlls to build directory for debugger
    file( COPY ${QT5_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${HDF5_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${ZLIB_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${LIBCRYPTO_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${LIBCURL_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${SZIP_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${LIBSSH_FILES_TO_COPY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

    if(isDebugBuild)
        file( COPY ${QT5_REQUISITE_DLL_PATH}/../plugins/platforms/qwindowsd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/platforms/ )
        file( COPY ${QUAZIP_LIBRARIES_DIR}/quazip1-qt5d.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
        file( COPY ${QWT_LIBRARY_DIRS}/qwtd.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    else()
        file( COPY ${QT5_REQUISITE_DLL_PATH}/../plugins/platforms/qwindows.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/platforms/ )
        file( COPY ${QUAZIP_LIBRARIES_DIR}/quazip1-qt5.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
        file( COPY ${QWT_LIBRARY_DIRS}/qwt.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    endif()

endif(WIN32)

#----------------------------------------
# Generate SALSA User Manual
#----------------------------------------

set(SALSA_USER_MANUAL_PATH ${CMAKE_CURRENT_LIST_DIR}/doc/manual/UserManual)
file( GLOB_RECURSE SALSA_USER_MANUAL_TEX "${SALSA_USER_MANUAL_PATH}/*.tex" )
file( GLOB_RECURSE SALSA_USER_MANUAL_BIB "${SALSA_USER_MANUAL_PATH}/*.bib" )
file( GLOB_RECURSE SALSA_USER_MANUAL_DIRTREE "${SALSA_USER_MANUAL_PATH}/dirtree*" )
file( GLOB_RECURSE SALSA_USER_MANUAL_FIGURES "${SALSA_USER_MANUAL_PATH}/figures/*" )

set( SALSA_USER_MANUAL_SOURCES
     ${SALSA_USER_MANUAL_TEX}
     ${SALSA_USER_MANUAL_BIB}
     ${SALSA_USER_MANUAL_DIRTREE}
     ${SALSA_USER_MANUAL_FIGURES} )

set(SALSA_TEX SalsaUserManual)
set(PDFLATEX_OPTIONS "-interaction=nonstopmode")

file( COPY ${SALSA_USER_MANUAL_PATH} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

add_custom_command(OUTPUT SalsaUserManual.pdf
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/UserManual
                   COMMAND pdflatex ${PDFLATEX_OPTIONS} ${SALSA_TEX} > pdflatex.log
                   COMMAND bibtex ${SALSA_TEX} || echo 1 > bibtex.log
                   COMMAND pdflatex ${PDFLATEX_OPTIONS} ${SALSA_TEX} >> pdflatex.log
                   COMMAND pdflatex ${PDFLATEX_OPTIONS} ${SALSA_TEX} >> pdflatex.log
                   COMMAND ${CMAKE_COMMAND} -E copy SalsaUserManual.pdf ${CMAKE_CURRENT_BINARY_DIR}/SalsaUserManual.pdf
                   DEPENDS ${SALSA_USER_MANUAL_SOURCES} )
add_custom_target(usermanual ALL DEPENDS SalsaUserManual.pdf )


#----------------------------------------
# Generate performance results
#----------------------------------------

if(IS_ARL_BUILD)
    add_custom_command(OUTPUT performance.png COMMAND python3 ${CMAKE_CURRENT_LIST_DIR}/scripts/parse_benchmarks_test.py --out performance.png DEPENDS ${BENCHMARKS_NAME} )
    add_custom_target(performance DEPENDS performance.png )
    set_target_properties(performance PROPERTIES EXCLUDE_FROM_ALL TRUE)
endif()

#----------------------------------------
# Install SALSA User Manual, 3rd party license info, converters, examples, common scripts
#----------------------------------------
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/SalsaUserManual.pdf
         DESTINATION doc )

install( FILES ${CMAKE_CURRENT_LIST_DIR}/doc/lsaValidationSpec.xlsx
         DESTINATION doc )

install( FILES ${CMAKE_CURRENT_LIST_DIR}/COPYING
         DESTINATION . )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doc/3rd_party
         DESTINATION doc )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/scripts/converters
         DESTINATION scripts/ )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/scripts/geoid
         DESTINATION scripts/ )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/scripts/common
         DESTINATION scripts/ )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/scripts/msi
         DESTINATION scripts/ )

install( DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/examples
         DESTINATION . )

#----------------------------------------
# Install Marble DLLs
#----------------------------------------

set( MARBLE_PLUGINS
         AnnotatePlugin
         AtmospherePlugin
         CompassFloatItem
         CrosshairsPlugin
         CycleStreetsPlugin
         EarthquakePlugin
         EclipsesPlugin
         ElevationProfileFloatItem
         ElevationProfileMarker
         FlightGearPositionProviderPlugin
         FoursquarePlugin
         GosmoreReverseGeocodingPlugin
         GosmoreRoutingPlugin
         GpsbabelPlugin
         GpsInfo
         GpxPlugin
         GraticulePlugin
         HostipPlugin
         JsonPlugin
         KmlPlugin
         LatLonPlugin
         License
         LocalDatabasePlugin
         LocalOsmSearchPlugin
         LogPlugin
         MapQuestPlugin
         MapScaleFloatItem
         MeasureTool
         MonavPlugin
         NavigationFloatItem
         NominatimReverseGeocodingPlugin
         NominatimSearchPlugin
         OpenRouteServicePlugin
         OsmPlugin
         OSRMPlugin
         OverviewMap
         Pn2Plugin
         PntPlugin
         PositionMarker
         PostalCode
         ProgressFloatItem
         RoutingPlugin
         RoutinoPlugin
         SatellitesPlugin
         Speedometer
         StarsPlugin
         SunPlugin
         YoursPlugin )

if(WIN32)
    string(REGEX REPLACE "\\\\" "/" CLEANED_MARBLE_LIBRARY_DIRS ${MARBLE_LIBRARY_DIRS})
    string(REGEX REPLACE "\\\\" "/" CLEANED_MARBLE_LIBRARY_DIRS_DEBUG ${MARBLE_LIBRARY_DIRS_DEBUG})

    install( FILES ${CLEANED_MARBLE_LIBRARY_DIRS}/lib/marblewidget-qt5.dll ${CLEANED_MARBLE_LIBRARY_DIRS}/lib/astro.dll
                 DESTINATION bin )

    listappend( MARBLE_RELEASE_PLUGINS ".dll" ${MARBLE_PLUGINS} )
        listappend( MARBLE_DEBUG_PLUGINS "d.dll" ${MARBLE_PLUGINS} )
        prepend( MARBLE_RELEASE_PLUGINS_TO_COPY ${CLEANED_MARBLE_LIBRARY_DIRS}/plugins/ ${MARBLE_RELEASE_PLUGINS} )
    prepend( MARBLE_DEBUG_PLUGINS_TO_COPY ${CLEANED_MARBLE_LIBRARY_DIRS_DEBUG}/plugins/ ${MARBLE_DEBUG_PLUGINS} )

        # message(STATUS "THINGS TO INSTALL:${MARBLE_PLUGINS_TO_COPY}}")

        install( FILES ${MARBLE_RELEASE_PLUGINS_TO_COPY} DESTINATION plugins/marble)

    # copy dlls to build directory for development
    file( COPY ${CLEANED_MARBLE_LIBRARY_DIRS}/lib/marblewidget-qt5.dll ${CLEANED_MARBLE_LIBRARY_DIRS}/lib/astro.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${CLEANED_MARBLE_LIBRARY_DIRS}/plugins ${CLEANED_MARBLE_LIBRARY_DIRS}/lib/astro.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
    file( COPY ${MARBLE_RELEASE_PLUGINS_TO_COPY} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/plugins/marble)

        if(EXISTS ${MARBLE_LIBRARY_DIRS_DEBUG})
                message( STATUS "DIRECTORY EXISTS")
                file( COPY ${CLEANED_MARBLE_LIBRARY_DIRS_DEBUG}/lib/marblewidget-qt5d.dll ${CLEANED_MARBLE_LIBRARY_DIRS_DEBUG}/lib/astrod.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
                file( COPY ${CLEANED_MARBLE_LIBRARY_DIRS_DEBUG}/plugins ${CLEANED_MARBLE_LIBRARY_DIRS_DEBUG}/lib/astrod.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
                file( COPY ${MARBLE_DEBUG_PLUGINS_TO_COPY} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/plugins/marble)
        else()
                message( STATUS "DIRECTORY DOES NOT EXIST")
        endif()
else()

        listappend( MARBLE_PLUGINS ".so" ${MARBLE_PLUGINS} )
        prepend( MARBLE_PLUGINS_TO_COPY ${MARBLE_LIBRARY_DIRS}/marble/plugins/lib ${MARBLE_PLUGINS} )

        install( FILES ${MARBLE_PLUGINS_TO_COPY} DESTINATION plugins/marble)
        file( COPY ${MARBLE_PLUGINS_TO_COPY} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/plugins/marble)
endif(WIN32)

#----------------------------------------
# Purge __pycache__ locations
#----------------------------------------

set(LSA_PYCACHE_LOCATIONS
    python/Lib/collections/__pycache__
    python/Lib/ctypes/__pycache__
    python/Lib/encodings/__pycache__
    python/Lib/__pycache__
    scripts/build/__pycache__
    scripts/converters/wrappers/__pycache__
    scripts/converters/__pycache__
    scripts/geoid/__pycache__
    scripts/reporting/__pycache__
    scripts/msi/__pycache__
    scripts/test/__pycache__ )


prepend( LSA_PYCACHE_LOCATIONS ${CMAKE_CURRENT_LIST_DIR}/ ${LSA_PYCACHE_LOCATIONS} )

function( removedirectories )
    foreach(DIRECTORY_TO_DELETE ${ARGN})

        add_custom_command(
                TARGET ${GUI_NAME}
                POST_BUILD
                COMMAND cmake -E remove_directory ${DIRECTORY_TO_DELETE}
                )

    endforeach(DIRECTORY_TO_DELETE)
endfunction( removedirectories )

removedirectories(${LSA_PYCACHE_LOCATIONS})


#----------------------------------------
# include(CPack)
#----------------------------------------
include(CPack)

#----------------------------------------
# Define public tests
#----------------------------------------
include(CTest)

# Call a python script that scrapes testpubliclsagui.cpp for all of the test names
execute_process(COMMAND ${PYTHON_EXE} ${CMAKE_CURRENT_LIST_DIR}/scripts/build/scrapePublicTestNames.py OUTPUT_VARIABLE pythonResponse )

# Convert the pythonReponse to a cmake list of strings
string(REPLACE " " ";" LIST_OF_GUI_TESTS "${pythonResponse}")

# Add a test for each member of the list
foreach(test ${LIST_OF_GUI_TESTS})
    ADD_TEST( NAME "gui_${test}" COMMAND ${TESTPUBLIC_NAME} ${test} )
endforeach(test)

##----------------------------------------
## Define internal tests
##----------------------------------------
if(IS_ARL_BUILD)
    include(CMakeLists_InternalTests.txt)
endif()

