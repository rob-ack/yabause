message(STATUS "Your C++ compiler supports these C++ features:\n${CMAKE_CXX_COMPILE_FEATURES}")
message(STATUS "Your C compiler supports these C features:\n${CMAKE_C_COMPILE_FEATURES}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

option(ENABLE_CONCURENCY_VISUALIZER_SUPPORT "Adds concurency visualizer sdk support" off)
mark_as_advanced(ENABLE_CONCURENCY_VISUALIZER_SUPPORT)
if(ENABLE_CONCURENCY_VISUALIZER_SUPPORT)
	add_definitions(-DCV_SUPPORT)
	include_directories(${CMAKE_SOURCE_DIR}/src/tools/cv/Inc)
endif()
