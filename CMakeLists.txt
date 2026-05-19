cmake_minimum_required(VERSION 4.1)
project(8080emu C)

set(CMAKE_C_STANDARD 11)

add_executable(8080emu
    main.c
    cpu.c
    bus.c
    instructions.c
    timing.c
)
