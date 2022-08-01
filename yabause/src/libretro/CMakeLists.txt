project(yabause)

cmake_minimum_required(VERSION 3.12)
cmake_policy(VERSION 3.12)

#this is because multiple ports are atm not buildable in parallel
set(YAB_PORTS "" CACHE STRING "List of ports to build" FORCE)

if(WIN32 OR LINUX)
	set(HAVE_CDROM OFF CACHE BOOL "Have CD-ROM support" FORCE)
endif()

if(HAVE_CDROM)
	add_definitions(-DHAVE_CDROM -DHAVE_FSEEKO)
endif()

set(LIBRETRO_COMM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libretro-common)
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

include_directories(${LIBRETRO_COMM_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(
	-D__LIBRETRO__
	-DVERSION=\"3.4.2\"
	-DHAVE_LROUND 
	-D_7ZIP_ST 
	-DFLAC__HAS_OGG=0 
	-DFLAC__NO_DLL
	-DNO_CLI 
	-DHAVE_BUILTIN_BSWAP16=1
	-DHAVE_BUILTIN_BSWAP32=1
	-DHAVE_C99_VARIADIC_MACROS=1
	-DHAVE_GETTIMEOFDAY=1
	-DHAVE_STDINT_H=1
	-DHAVE_SYS_TIME_H=1
	-DIMPROVED_SAVESTATES 
	-DPACKAGE_VERSION=\"1.3.3\"
	-DHAVE_FLOORF=1
	-DHAVE_GLSYM_PRIVATE
	-DHAVE_THREADS=1 
)

set(LIBRETRO_SRCS ${LIBRETRO_SRCS}
	glsym_private.c
	libretro.c
	${LIBRETRO_COMM_DIR}/compat/compat_posix_string.c
	${LIBRETRO_COMM_DIR}/compat/compat_strl.c
	${LIBRETRO_COMM_DIR}/compat/fopen_utf8.c
	${LIBRETRO_COMM_DIR}/encodings/encoding_utf.c
	${LIBRETRO_COMM_DIR}/file/file_path.c
	${LIBRETRO_COMM_DIR}/compat/compat_strcasestr.c
	${LIBRETRO_COMM_DIR}/vfs/vfs_implementation.c
	${LIBRETRO_COMM_DIR}/streams/file_stream.c
	${LIBRETRO_COMM_DIR}/rthreads/rthreads.c
	${LIBRETRO_COMM_DIR}/glsm/glsm.c 
	${LIBRETRO_COMM_DIR}/glsym/rglgen.c 
	${LIBRETRO_COMM_DIR}/string/stdstring.c
)

if(HAVE_CDROM)
	set(LIBRETRO_SRCS ${LIBRETRO_SRCS}
		${LIBRETRO_COMM_DIR}/vfs/vfs_implementation_cdrom.c 
		${LIBRETRO_COMM_DIR}/cdrom/cdrom.c 
		${LIBRETRO_COMM_DIR}/memmap/memalign.c 
		${LIBRETRO_COMM_DIR}/lists/string_list.c
		${LIBRETRO_COMM_DIR}/lists/dir_list.c 
		${LIBRETRO_COMM_DIR}/string/stdstring.c
		${LIBRETRO_COMM_DIR}/file/retro_dirent.c
		${SOURCE_DIR}/cd-libretro.c
	)
endif()

if(WIN32)
	add_definitions(-D_WINDOWS)
endif()

if (YAB_FORCE_GLES31 OR OPENGLES31_FOUND OR OPENGLES3_FOUND)
	message(STATUS "OpenGL ES 3.1 found, using GLES 3.1")
	set(LIBRETRO_SRCS ${LIBRETRO_SRCS} ${LIBRETRO_COMM_DIR}/glsym/glsym_es3.c)
	add_definitions(
		-D_OGLES3_
	)
	if (NOT OPENGLES31_FOUND)
		if(WIN32)
			message(WARNING "Failed to find OpenGLES. Targeting OpenGLES on Windows is not nessesarly usefull since most Windows systems wont support OpenGLES and rather support OpenGL. But try to workaround that anyways.")
			SET (OPENGLES_gl_LIBRARY import32 CACHE STRING "OpenGL ES 1.x library for win32")
		endif()
	endif()
else()
	message(STATUS "OpenGL ES 3.1 not found, using OpenGL")
	set(LIBRETRO_SRCS ${LIBRETRO_SRCS} ${LIBRETRO_COMM_DIR}/glsym/glsym_gl.c)
endif()

set(LIBRETRO_PUBLIC_HDRS
#	libretro_core_options.h
)

set(LIBRETRO_PRIVATE_HDRS
	${LIBRETRO_COMM_DIR}/include/compat/msvc.h
	glsym_private.h
)

if(FORCE_CXX_COMPILER)
	SET_SOURCE_FILES_PROPERTIES( ${LIBRETRO_SRCS} PROPERTIES LANGUAGE CXX )
endif()

add_library(yabause_libretro SHARED ${LIBRETRO_SRCS} 
									${LIBRETRO_PUBLIC_HDRS} 
									${LIBRETRO_PRIVATE_HDRS} 
									${M68KMAKE_INC_SOURCES} 
									${GEN68K_INC_SOURCES}
)

add_dependencies(yabause_libretro yabause)
add_dependencies(yabause_libretro zlib)
add_dependencies(yabause_libretro libchdr)

#the order is important for now which is just about the order of linking. some of the components still need to be exchanged when building a libretrocore
target_link_libraries(yabause_libretro yabause ${zlib_STATIC_LIBRARIES} ${LIBCHDR_LIBRARIES} ${YABAUSE_LIBRARIES})

if(YAB_FORCE_GLES31 OR OPENGLES31_FOUND OR OPENGLES3_FOUND)
	target_link_libraries(yabause_libretro GLESv2)
endif()

if(WIN32)
	target_link_libraries(yabause_libretro opengl32.lib ws2_32.lib)
endif()

if(UNIX)
	target_compile_options(mini18n-static PUBLIC -fPIC)
	target_compile_options(zlibstatic PUBLIC -fPIC)
	target_compile_options(yabause PUBLIC -fPIC)
	target_compile_options(yabause_libretro PUBLIC -fPIC)
endif()