cmake_minimum_required(VERSION 3.15)
project(Helix CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE git2 )
target_compile_options(${PROJECT_NAME} PRIVATE -O3 -Wall )
