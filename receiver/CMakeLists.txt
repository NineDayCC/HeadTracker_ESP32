cmake_minimum_required(VERSION 3.20.0)
set(BOARD esp32c3_devkitm)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(HeadTracker_ESP32_receiver)

zephyr_compile_options(-Wno-missing-braces)

FILE(GLOB app_sources src/*.c src/*/*.c)
target_sources(app PRIVATE ${app_sources})
