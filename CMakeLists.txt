cmake_minimum_required(VERSION 3.16)
project(RISC_V_2022)
set(CMAKE_CXX_FLAGS "-std=c++17 -Ofast")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Wall -fsanitize=address -fsanitize=undefined")

add_executable(code src/main.cpp)