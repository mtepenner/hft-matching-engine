cmake_minimum_required(VERSION 3.20)
project(exchange_core CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HFT_NATIVE_OPTIMIZATION "Enable CPU-specific optimizations" OFF)

add_compile_options(-O3 -fno-omit-frame-pointer)

if(HFT_NATIVE_OPTIMIZATION)
    add_compile_options(-march=native -mtune=native)
endif()

include_directories(include)

add_executable(exchange_core
    src/main.cpp
    src/engine/order_book.cpp
    src/engine/matcher.cpp
    src/network/kernel_bypass_udp.cpp
)

target_compile_definitions(exchange_core PRIVATE
    CACHE_LINE_SIZE=64
    USE_CPU_PINNING=1
)
