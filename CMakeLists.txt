cmake_minimum_required(VERSION 3.18)
project(tasbot_eyes C)
set(CMAKE_C_STANDARD 11)

add_executable(tasbot_eyes main.c)

target_link_libraries(tasbot_eyes gif)
target_link_libraries(tasbot_eyes ws2811)
target_link_libraries(tasbot_eyes m) #libm, screw you