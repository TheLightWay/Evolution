
set(ZLIB_VERSION "1.2.11")

option(ASM686 "Enable building i686 assembly implementation")
option(AMD64 "Enable building amd64 assembly implementation")

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(stdint.h    HAVE_STDINT_H)
check_include_file(stddef.h    HAVE_STDDEF_H)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
if(HAVE_SYS_TYPES_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_SYS_TYPES_H)
endif()
if(HAVE_STDINT_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDINT_H)
endif()
if(HAVE_STDDEF_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -DHAVE_STDDEF_H)
endif()
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable

#
# Check for fseeko
#
check_function_exists(fseeko HAVE_FSEEKO)
if(NOT HAVE_FSEEKO)
    add_definitions(-DNO_FSEEKO)
endif()

#
# Check for unistd.h
#
check_include_file(unistd.h Z_HAVE_UNISTD_H)

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file( ${CMAKE_CURRENT_LIST_DIR}/zlib.pc.cmakein
		${ZLIB_PC} @ONLY)
configure_file(	${CMAKE_CURRENT_LIST_DIR}/zconf.h.cmakein
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

set( ZSRCDIR ${CMAKE_CURRENT_LIST_DIR} )

set(ZLIB_PUBLIC_HDRS
        ${CMAKE_CURRENT_BINARY_DIR}/zconf.h
        ${ZSRCDIR}/zlib.h
)

set(ZLIB_PRIVATE_HDRS
    ${ZSRCDIR}/crc32.h
    ${ZSRCDIR}/deflate.h
    ${ZSRCDIR}/gzguts.h
    ${ZSRCDIR}/inffast.h
    ${ZSRCDIR}/inffixed.h
    ${ZSRCDIR}/inflate.h
    ${ZSRCDIR}/inftrees.h
    ${ZSRCDIR}/trees.h
    ${ZSRCDIR}/zutil.h
)
set(ZLIB_SRCS
    ${ZSRCDIR}/adler32.c
    ${ZSRCDIR}/compress.c
    ${ZSRCDIR}/crc32.c
    ${ZSRCDIR}/deflate.c
    ${ZSRCDIR}/gzclose.c
    ${ZSRCDIR}/gzlib.c
    ${ZSRCDIR}/gzread.c
    ${ZSRCDIR}/gzwrite.c
    ${ZSRCDIR}/inflate.c
    ${ZSRCDIR}/infback.c
    ${ZSRCDIR}/inftrees.c
    ${ZSRCDIR}/inffast.c
    ${ZSRCDIR}/trees.c
    ${ZSRCDIR}/uncompr.c
    ${ZSRCDIR}/zutil.c
)


if(CMAKE_COMPILER_IS_GNUCC)
    if(ASM686)
        set(ZLIB_ASMS ${ZSRCDIR}/contrib/asm686/match.S)
    elseif (AMD64)
        set(ZLIB_ASMS ${ZSRCDIR}/contrib/amd64/amd64-match.S)
    endif ()

	if(ZLIB_ASMS)
		add_definitions(-DASMV)
		set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE C COMPILE_FLAGS -DNO_UNDERLINE)
	endif()
endif()

if(MSVC)
    if(ASM686)
		ENABLE_LANGUAGE(ASM_MASM)
        set(ZLIB_ASMS
            ${ZSRCDIR}/contrib/masmx86/inffas32.asm
			${ZSRCDIR}/contrib/masmx86/match686.asm
		)
    elseif (AMD64)
		ENABLE_LANGUAGE(ASM_MASM)
        set(ZLIB_ASMS
            ${ZSRCDIR}/contrib/masmx64/gvmat64.asm
			${ZSRCDIR}/contrib/masmx64/inffasx64.asm
		)
    endif()

	if(ZLIB_ASMS)
		add_definitions(-DASMV -DASMINF)
	endif()
endif()

add_library( zlib STATIC ${ZLIB_SRCS} ${ZLIB_ASMS} ${ZLIB_PUBLIC_HDRS} ${ZLIB_PRIVATE_HDRS} )
target_include_directories( zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_LIST_DIR} )