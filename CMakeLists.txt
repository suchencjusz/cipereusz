cmake_minimum_required(VERSION 3.15)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

project(cipereusz)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


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
)

target_link_libraries(cipereusz PRIVATE dpp)