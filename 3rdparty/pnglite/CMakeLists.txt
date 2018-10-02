
set( SRCLIST "${CMAKE_CURRENT_LIST_DIR}/pnglite.c" )
set( INCLIST "${CMAKE_CURRENT_LIST_DIR}/pnglite.h" )

set( THIS_TARGET_SOURCES ${SRCLIST} ${INCLIST} )

set( ZLIB_INCLUDE_DIRS "${TOP_LEVEL_SOURCE_DIR}/3rdparty/zlib")

add_library( pnglite STATIC ${THIS_TARGET_SOURCES} )
target_include_directories( pnglite PUBLIC "${CMAKE_CURRENT_LIST_DIR}"  )
target_include_directories( pnglite PRIVATE ${ZLIB_INCLUDE_DIRS}  )
target_link_libraries( pnglite PRIVATE zlib )

#SetupCompilerWarnings( pnglite )

