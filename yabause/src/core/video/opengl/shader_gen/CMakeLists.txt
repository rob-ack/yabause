project (shader_gen)

option(ROB_SHADER_GEN "A Programm which allows to output the shaders used in KRONOS to glsl files which are human readable. It will require c++ 20." off)
mark_as_advanced(ROB_SHADER_GEN)

if(NOT ROB_SHADER_GEN)
    return()
endif()

include_directories(${CMAKE_SOURCE_DIR}/core/video/opengl)
set(CMAKE_CXX_STANDARD 23)

set(SHADER_GEN_SOURCES
    main.cpp
    gen.cpp
    gen.hpp
)

add_executable(shader_gen ${SHADER_GEN_SOURCES})

add_custom_command(
    COMMAND shader_gen ${CMAKE_CURRENT_SOURCE_DIR}/shaders/
    DEPENDS shader_gen
    OUTPUT ""
    COMMENT "Generating OpenGL Shader Files"
)