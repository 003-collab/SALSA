set( EIGEN3_FOUND FALSE )
message(STATUS "")

if( DEFINED ENV{eigen3} )
    message(STATUS "Searching for EIGEN3 location defined by environment variable $ENV{eigen}" )
    set( EIGEN3_INCLUDE_DIRS $ENV{eigen3} )
else()
    message(STATUS "Searching for default Eigen3 location $ENV{HOME}/.local/eigen3..." )
    set( EIGEN3_INCLUDE_DIRS $ENV{HOME}/.local/eigen3/include/eigen3 )
endif()

if(EXISTS ${EIGEN3_INCLUDE_DIRS})
  set( EIGEN3_FOUND TRUE )
  message(STATUS "EIGEN3_INCLUDE_DIRS: ${EIGEN3_INCLUDE_DIRS}")
else()
    message(WARNING "Could not find eigen3 at ${EIGEN3_INCLUDE_DIRS}.")
endif()

message(STATUS "EIGEN3_FOUND: ${EIGEN3_FOUND}")

return( )
