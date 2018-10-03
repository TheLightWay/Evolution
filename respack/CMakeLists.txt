
set( PNGLITE_INCLUDE_DIR "${TOP_LEVEL_SOURCE_DIR}/3rdparty/pnglite")

set( RESPACKSRC 
    ${CMAKE_CURRENT_LIST_DIR}/CLI11.hpp
    ${CMAKE_CURRENT_LIST_DIR}/stb_image.h
    ${CMAKE_CURRENT_LIST_DIR}/stb_image.c   
    ${CMAKE_CURRENT_LIST_DIR}/respack.cpp    
)
add_executable( respack ${RESPACKSRC} )
#SetupCompilerWarnings( respack )