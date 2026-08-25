cmake_minimum_required(VERSION 3.16)
project(pcx_test_package C)

find_package(pcx CONFIG REQUIRED)

add_executable(pcx_test_package src/main.c)
target_link_libraries(pcx_test_package PRIVATE pcx::pcx)
