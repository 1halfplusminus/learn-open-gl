cmake_minimum_required(VERSION 3.0.0)
project(openglc VERSION 0.1.0)

find_package(OpenGL REQUIRED)

include(CTest)
enable_testing()

INCLUDE_DIRECTORIES(./glfw/include)
INCLUDE_DIRECTORIES(./glad/include)
INCLUDE_DIRECTORIES(./include)

add_subdirectory (glfw)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
)

add_library(test_library STATIC ./glad/src/glad.c ./src/shader)

add_executable(openglc main.cpp)

target_link_libraries(openglc ${ALL_LIBS} test_library)

file(COPY "./shader" DESTINATION  "./Debug")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
