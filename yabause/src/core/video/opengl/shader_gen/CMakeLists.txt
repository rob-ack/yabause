project (shader_gen)

include_directories(${CMAKE_SOURCE_DIR}/core/video/opengl)

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