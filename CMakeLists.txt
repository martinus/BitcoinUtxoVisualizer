# see https://github.com/rvarago/modern-cmake-template
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

project(buv)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# use ccache, needs to be done before add_executable
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    SET(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif(CCACHE_PROGRAM)

# configuration see .clang-tidy
#if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#    find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy)
#    if(CLANG_TIDY_PROGRAM)
#        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
#    endif()
#endif()


add_executable(${PROJECT_NAME} "")

# needs OpenCV
find_package(OpenCV REQUIRED)
MESSAGE(STATUS "OpenCV ${OpenCV_VERSION}")
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

add_subdirectory(src)

target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy)
