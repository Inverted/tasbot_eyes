cmake_minimum_required(VERSION 3.16)
project(tasbot_eyes C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(tasbot_eyes main.c color.c color.h animation_stack.c animation_stack.h utils.h)

target_link_libraries(tasbot_eyes gif)
target_link_libraries(tasbot_eyes ws2811)
target_link_libraries(tasbot_eyes m) #libm, screw you