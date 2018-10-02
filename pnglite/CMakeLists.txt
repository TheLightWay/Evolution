#find of build zlib library (for png)
find_package( zlib QUIET )
if( NOT ZLIB_FOUND )
    BuildExternalProjectFromGit( zlib "https://github.com/madler/zlib.git" "master" )
    set( ZLIB_ROOT "${INSTALL_THRIDPARTY_DIR}/zlib" )
    find_package( zlib REQUIRED )
endif()


set( SRCLIST "${CMAKE_CURRENT_LIST_DIR}/pnglite.c" )
set( INCLIST "${CMAKE_CURRENT_LIST_DIR}/pnglite.h" )

set( THIS_TARGET_SOURCES ${SRCLIST} ${INCLIST} )

add_library( pnglite STATIC ${THIS_TARGET_SOURCES} )
target_include_directories( pnglite PUBLIC "${CMAKE_CURRENT_LIST_DIR}"  )
target_include_directories( pnglite PRIVATE ${ZLIB_INCLUDE_DIRS}  )
target_link_libraries( pnglite PRIVATE ${ZLIB_LIBRARIES} )

#SetupCompilerWarnings( pnglite )

