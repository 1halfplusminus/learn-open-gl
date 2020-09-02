cmake_minimum_required(VERSION 3.0.0)
project(openglc VERSION 0.1.0)

find_package(OpenGL REQUIRED)

include(CTest)
enable_testing()

INCLUDE_DIRECTORIES(./glfw/include)
INCLUDE_DIRECTORIES(./glad/include)
INCLUDE_DIRECTORIES(./include)
INCLUDE_DIRECTORIES(./stb)

add_subdirectory (glfw)
add_subdirectory (glm)
add_subdirectory (assimp)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	glm
)
set(HEADER_FILES ./stb/stb_image.h ./include/engine.h)

add_library(test_library STATIC ./glad/src/glad.c ./src/shader )
target_include_directories(test_library PRIVATE ./stb)

add_executable(openglc main.cpp )
target_link_libraries(openglc ${ALL_LIBS} test_library)

add_executable(texture texture.cpp ${HEADER_FILES})
target_link_libraries(texture ${ALL_LIBS} test_library)


add_executable(transform transform.cpp ${HEADER_FILES})
target_link_libraries(transform ${ALL_LIBS} test_library)

add_executable(coordinate_systems coordinate_systems.cpp ${HEADER_FILES})
target_link_libraries(coordinate_systems ${ALL_LIBS} test_library)

add_executable(camera camera.cpp ${HEADER_FILES})
target_link_libraries(camera ${ALL_LIBS} test_library)

add_executable(get_started get_started.cpp ${HEADER_FILES})
target_link_libraries(get_started ${ALL_LIBS} test_library)

add_executable(lighting_color lighting_color.cpp ${HEADER_FILES})
target_link_libraries(lighting_color ${ALL_LIBS} test_library)

add_executable(model_loading model_loading.cpp ${HEADER_FILES})
target_link_libraries(model_loading ${ALL_LIBS} test_library)

add_executable(sprite_renderer sprite_renderer.cpp ${HEADER_FILES})
target_link_libraries(sprite_renderer ${ALL_LIBS} test_library)

file(COPY "./shader" DESTINATION  "./Debug")
file(COPY "./texture" DESTINATION  "./Debug")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
