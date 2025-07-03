# Find GNSSTk
#
# Find the native GNSSTk includes and library.
# Currently just hard-coded paths
#
#  GNSSTK_INCLUDE_DIRS   - where to find GNSSTK files.hpp, etc.
#  GNSSTK_LIBRARIES      - List of libraries when using GNSSTK.
#  GNSSTK_FOUND          - True if GNSSTK was "found". So, always true.
#
#=============================================================================
set( GNSSTK_FOUND FALSE )
message(STATUS "")

if( DEFINED ENV{gnsstk_salsa} )
    message(STATUS "Searching for GNSSTK location defined by environment variable $ENV{gnsstk_salsa}..." )
    set( RELEASE_DIR $ENV{gnsstk_salsa} )
else()
    if( UNIX )
        message(STATUS "Searching for default GNSSTk location $ENV{HOME}/.local/gnsstk..." )
        set( RELEASE_DIR $ENV{HOME}/.local/gnsstk )
    elseif( WIN32 )
        message(STATUS "Searching for default GNSSTk location $ENV{USERPROFILE}/.local/gnsstk..." )
        set( RELEASE_DIR $ENV{USERPROFILE}/.local/gnsstk )
    endif()
    message(STATUS "SETTING RELEASE_DIR TO ${RELEASE_DIR}" )
endif()

set( DEBUG_DIR ${RELEASE_DIR}_debug )

if( UNIX)
    # Release library
    if( EXISTS ${RELEASE_DIR} )
        set( GNSSTK_INCLUDE_DIRS "${RELEASE_DIR}/include/gnsstk" )
        set( GNSSTK_LIBRARIES    "${RELEASE_DIR}/lib/libgnsstk.so" )
        set( GNSSTK_LIBRARY_DIRS "${RELEASE_DIR}/lib" )
    endif()
    # Debug library
    if( EXISTS ${DEBUG_DIR} )
        set( GNSSTK_INCLUDE_DIRS_DEBUG "${DEBUG_DIR}/include/gnsstk" )
        set( GNSSTK_LIBRARIES_DEBUG    "${DEBUG_DIR}/lib/libgnsstk.so" )
        set( GNSSTK_LIBRARY_DIRS_DEBUG "${DEBUG_DIR}/lib" )
    endif()
elseif( WIN32 )
    # Release library
    if( EXISTS ${RELEASE_DIR} )
        set( GNSSTK_INCLUDE_DIRS "${RELEASE_DIR}/include/gnsstk" )
        set( GNSSTK_LIBRARIES    "${RELEASE_DIR}/lib/gnsstk.lib" )
        set( GNSSTK_LIBRARY_DIRS "${RELEASE_DIR}/lib" )
    else()
        message(STATUS "${RELEASE_DIR} DOESN'T EXIST." )
    endif()
    # Debug library
    if( EXISTS ${DEBUG_DIR} )
        set( GNSSTK_INCLUDE_DIRS_DEBUG "${DEBUG_DIR}/include/gnsstk" )
        set( GNSSTK_LIBRARIES_DEBUG    "${DEBUG_DIR}/lib/gnsstk.lib" )
        set( GNSSTK_LIBRARY_DIRS_DEBUG "${DEBUG_DIR}/lib" )
    else()
        message(STATUS "${DEBUG_DIR} DOESN'T EXIST." )
    endif()
endif()

message (STATUS "GNSSTK_LIBRARIES:    ${GNSSTK_LIBRARIES}")
message (STATUS "GNSSTK_INCLUDE_DIRS: ${GNSSTK_INCLUDE_DIRS}")
message (STATUS "GNSSTK_LIBRARY_DIRS: ${GNSSTK_LIBRARY_DIRS}")

message (STATUS "GNSSTK_LIBRARIES_DEBUG:    ${GNSSTK_LIBRARIES_DEBUG}")
message (STATUS "GNSSTK_INCLUDE_DIRS_DEBUG: ${GNSSTK_INCLUDE_DIRS_DEBUG}")
message (STATUS "GNSSTK_LIBRARY_DIRS_DEBUG: ${GNSSTK_LIBRARY_DIRS_DEBUG}")

if( ( EXISTS ${GNSSTK_LIBRARIES} ) OR ( EXISTS ${GNSSTK_LIBRARIES_DEBUG} ) )
  set( GNSSTK_FOUND TRUE )
else()
    message(WARNING "Could not find gnsstk at ${GNSSTK_LIBRARIES}.")
    message(STATUS  "Verify gnsstk is installed in one of these locations:")
    message(STATUS  "    1) Location defined in gnsstk env variable.")
    message(STATUS  "       Example: $ export gnsstk=~/git/gnsstk/dev/install")
    message(STATUS  "    2) Default gnsstk install location of (linux) $HOME/.local/gnsstk/ or (win32) C:/Program Files/gnsstk")
endif()

message(STATUS "GNSSTK_FOUND: ${GNSSTK_FOUND}")

return( )
