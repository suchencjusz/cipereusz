cmake_minimum_required(VERSION 3.15)
project(cipereusz)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(dpp_DIR ${PROJECT_SOURCE_DIR}/vendor/dpp/lib/cmake/dpp)

find_package(dpp 10.1.3 REQUIRED)

add_executable(cipereusz main.cpp
        MarkovChains.h
        MarkovChains.cpp
        Config.h
        Utils.h
        MarkovChainsNGram.h
        MarkovChainsNGram.cpp
)

target_link_libraries(cipereusz PRIVATE dpp::dpp)