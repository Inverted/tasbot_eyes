cmake_minimum_required(VERSION 3.16)
project(tasbot_eyes C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(tasbot_eyes
        main.c
        color.c
        color.h
        gif.c
        gif.h
        led.c
        led.h
        arguments.c
        arguments.h
        tasbot.c
        tasbot.h
        palette.c
        palette.h
        filesystem.c
        filesystem.h
        utils.c
        utils.h
        network.c
        network.h
        stack.c
        stack.h
        )

target_link_libraries(tasbot_eyes gif)
target_link_libraries(tasbot_eyes ws2811)
target_link_libraries(tasbot_eyes pthread)
target_link_libraries(tasbot_eyes m) #libm, screw you