cmake_minimum_required(VERSION 3.15)
project(Helix CXX)


file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE git2 )
