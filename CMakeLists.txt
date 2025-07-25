cmake_minimum_required(VERSION 3.14)
project(TungstenCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(TungstenCore
    include/TungstenCore/TungstenCore.hpp
    src/TungstenCore.cpp
)

target_include_directories(TungstenCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(TungstenCore PUBLIC TungstenUtils)