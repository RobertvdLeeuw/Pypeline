cmake_minimum_required(VERSION 3.20)
project(CodeNote)

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_GENERATOR "Visual Studio 15 2017 Win64")
# set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/MD)
endif()


if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
else()
    message(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(WARNING "The file conanbuildinfo.cmake doesn't exist, you have to run conan install first")
endif()

add_executable(${PROJECT_NAME} main.cpp strman.cpp common.h gui.cpp core.cpp imgui_impl_glfw.cpp imgui_impl_glfw.h imgui_impl_opengl3.cpp imgui_impl_opengl3.h fileio.cpp)


# message(${CONAN_LIBS})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
