

set( QWT_FOUND FALSE )
message(STATUS "")
if( DEFINED ENV{QWT} )
    message(STATUS "Searching for QWT location defined by environment variable " $ENV{QWT} )
    set(QWT_INSTALL $ENV{QWT} )
else()
    message(STATUS "Searching for default QWT location: $ENV{HOME}/.local/qwt-6.2.0" )
    set(QWT_INSTALL $ENV{HOME}/.local/qwt-6.2.0)
endif()

set( QWT_INCLUDE_DIRS "${QWT_INSTALL}/include" )
set( QWT_LIBRARY_DIRS "${QWT_INSTALL}/lib" )
if( UNIX )
    set( QWT_LIBRARIES    "${QWT_INSTALL}/lib/libqwt.so" )
elseif( WIN32 )
    #release
    set( QWT_LIBRARIES    "${QWT_INSTALL}/lib/qwt.lib" )
    #debug
    set( QWT_LIBRARIES_DEBUG    "${QWT_INSTALL}/lib/qwtd.lib" )
endif()

message (STATUS "Searching for QWT_LIBRARIES at: ${QWT_LIBRARIES}")
if(EXISTS ${QWT_LIBRARIES})
    set( QWT_FOUND TRUE )
endif()
message(STATUS "QWT_FOUND: ${QWT_FOUND}")


if(NOT QWT_FOUND)
        message(WARNING "Could not find qwt at ${QWT_LIBRARIES}.")
        message(STATUS  "Verify qwt is installed in this location:")
        message(STATUS  "       Location specified by QWT_INSTALL_PREFIX in qwtconfig.pri.")
        message(STATUS  "       Example: $HOME/.local/qwt-6.2.0/lib")
endif()

return( )
