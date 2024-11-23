#--------------------------------------------------------------------------------------
# iOS build
#--------------------------------------------------------------------------------------
# Build step
# 1) mkdir build
# 2) cd build
# 3) cmake ../yabause -DGIT_EXECUTABLE=/usr/bin/git -DSH2_DYNAREC=OFF -DYAB_WANT_DYNAREC_DEVMIYAX=OFF -DCMAKE_TOOLCHAIN_FILE=../yabause/src/ios/iOS.cmake -DIOS_PLATFORM=OS -DYAB_PORTS=ios
# 4) make
# 5) open yabause/src/ios/ios/uoyabause/uoyabause.xcodeproj
# 6) >
#--------------------------------------------------------------------------------------


project(yabause-ios)

yab_port_start()

set( LIBRARY_NAME yabause_ios )


set( SOURCES 
 yui.cpp
 osdcore.h
 osdcore.c
)


set( LIBRARY_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/uoyabause/ )

add_library(${LIBRARY_NAME}  $<TARGET_OBJECTS:yabause>  ${SOURCES}  )
#set_xcode_property (${LIBRARY_NAME} GCC_GENERATE_DEBUGGING_SYMBOLS YES)
include_directories( ${YABAUSE_INCLUDE_DIR} ${PROJECT_BINARY_DIR}/../ ${PROJECT_SOURCE_DIR}/../ )
target_link_libraries(${LIBRARY_NAME}   ${YABAUSE_LIBRARIES} )

# Installation
#set (CMAKE_INSTALL_PREFIX "${hello-lib_SOURCE_DIR}/../hello-app/hello-app")
#install (TARGETS hello-lib DESTINATION .)
#install (FILES HelloIOS.h DESTINATION .)

add_custom_command(TARGET ${LIBRARY_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/src/zlib/install/lib/libz.a"
        "${CMAKE_SOURCE_DIR}/src/ios/uoyabause/"
    #COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #    "${CMAKE_BINARY_DIR}/src/libchdr-prefix/src/libchdr-build/libflac-static.a"
    #    "${CMAKE_SOURCE_DIR}/src/ios/uoyabause/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/src/libchdr-prefix/src/libchdr-build/deps/zstd-1.5.6/build/cmake/lib/libzstd.a"
        "${CMAKE_SOURCE_DIR}/src/ios/uoyabause/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/src/libchdr-prefix/src/libchdr-build/deps/lzma-24.05/liblzma.a"
        "${CMAKE_SOURCE_DIR}/src/ios/uoyabause/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/src/libchdr-prefix/src/libchdr-build/libchdr-static.a"
        "${CMAKE_SOURCE_DIR}/src/ios/uoyabause/"
    COMMENT "Copying libraries to uoyabause directory..."
)
