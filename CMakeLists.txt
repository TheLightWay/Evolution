#using cmake 3.7 
cmake_minimum_required( VERSION 3.7 FATAL_ERROR )

project( Evolution )

enable_language( C )
enable_language( CXX )
# Force C++11 support requirement
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )


set( TOP_LEVEL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" )

list( APPEND CMAKE_MODULE_PATH "${TOP_LEVEL_SOURCE_DIR}/cmake" )
list( APPEND CMAKE_MODULE_PATH "${TOP_LEVEL_SOURCE_DIR}/cmake/modules" )

set( SRCDIR "${TOP_LEVEL_SOURCE_DIR}/src" )

include( Functions )

DisallowIntreeBuilds( )

MakeOutputDirectories( )

if( MSVC )
	add_definitions( "-D_CRT_SECURE_NO_DEPRECATE" )
	add_definitions( "-D_CRT_SECURE_NO_WARNINGS" )
endif( )

#find or build SDL2 library
find_package( SDL2 QUIET )
if( NOT SDL2_FOUND )
    BuildExternalProjectFromGit( SDL-mirror "https://github.com/spurious/SDL-mirror.git" "release-2.0.8" )
    set( SDL2_PATH "${INSTALL_THRIDPARTY_DIR}/SDL-mirror" )
    find_package( SDL2 REQUIRED )
endif()

#build pnglite static library
add_subdirectory( pnglite )

#build respack utility

set( SHADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/shaders" )
set( IMAGES_DIR "${CMAKE_CURRENT_LIST_DIR}/images" )

file( GLOB SHADERS "${SHADERS_DIR}/*.vert" "${SHADERS_DIR}/*.frag" )
file( GLOB IMAGES "${IMAGES_DIR}/*.png" )

add_executable( respack "${SRCDIR}/respack.cpp" )
add_dependencies( respack pnglite )
target_include_directories( respack PRIVATE "${SRCDIR}/pnglite" )
target_link_libraries( respack pnglite )

set( GENRESSRCDIR "${PROJECT_BINARY_DIR}/gensrc" )
file( MAKE_DIRECTORY ${GENRESSRCDIR} )

add_custom_command( 
	OUTPUT ${GENRESSRCDIR}/resources.cpp ${GENRESSRCDIR}/resources.h
	COMMAND respack -d ${GENRESSRCDIR} -f resources -s ${SHADERS} -i ${IMAGES}
	DEPENDS respack
	VERBATIM
)
					

set( EVO_SRC_LIST 
        ${SRCDIR}/main.cpp
        ${SRCDIR}/graph.cpp
        ${SRCDIR}/graph.h
        ${SRCDIR}/hash.cpp
        ${SRCDIR}/hash.h
        ${SRCDIR}/world.cpp
        ${SRCDIR}/world.h
        ${SRCDIR}/stream.cpp
        ${SRCDIR}/stream.h
        ${SRCDIR}/evo_math.cpp
        ${SRCDIR}/evo_math.h
        ${SRCDIR}/video.h
        ${SRCDIR}/selection.cpp
        ${GENRESSRCDIR}/resources.h
        ${GENRESSRCDIR}/resources.cpp
)

add_executable( "Evolution" ${EVO_SRC_LIST} )
target_include_directories( "Evolution" 
        PRIVATE ${SRCDIR} ${GENRESSRCDIR} ${SDL2_INCLUDE_DIR} )
target_link_libraries( "Evolution" PRIVATE ${SDL2_LIBRARY} )








