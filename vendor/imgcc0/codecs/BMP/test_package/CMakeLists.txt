cmake_minimum_required(VERSION 3.16)
project(bmp_test_package C)

find_package(bmp CONFIG REQUIRED)

add_executable(bmp_test_package src/main.c)
target_link_libraries(bmp_test_package PRIVATE bmp::bmp)
