cmake_minimum_required(VERSION 2.8)

project(rpdiceroller)
add_executable(${PROJECT_NAME} "main.cpp")
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -Werror -pedantic -Wno-unused-variable")
