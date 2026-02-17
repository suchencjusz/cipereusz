cmake_minimum_required(VERSION 3.15)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE) # off bo moj serwer to i5 3gen

project(cipereusz)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-march=x86-64 -mtune=generic)

include(FetchContent)

FetchContent_Declare(
        dpp
        GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
        GIT_TAG        v10.1.3
)
FetchContent_MakeAvailable(dpp)

add_executable(cipereusz main.cpp
        MarkovChains.h
        MarkovChains.cpp
        Config.h
        Utils.h
        MarkovChainsNGram.h
        MarkovChainsNGram.cpp
        dpp_commands.cpp
        dpp_commands.h
)

target_link_libraries(cipereusz PRIVATE dpp)