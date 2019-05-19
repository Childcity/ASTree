cmake_minimum_required(VERSION 3.10)
project(ASTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

add_subdirectory(example)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
