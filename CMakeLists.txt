cmake_minimum_required(VERSION 3.16)

# Project name and language
project(AlifLSP VERSION 0.0.1 LANGUAGES CXX)

# Enforce C++20 standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create the executable target and add all source files
add_executable(alif-lsp
    src/AlifLSP.cpp
    src/Completion.cpp
    src/DocManager.cpp
    src/Logger.cpp
    src/Server.cpp
)

# Add include directories (headers and third-party)
target_include_directories(alif-lsp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third-party
)

# Set compiler-specific flags
if(MSVC)
    # Windows/MSVC: /utf-8 is required for Arabic characters in source files
    target_compile_options(alif-lsp PRIVATE /W3 /utf-8)
else()
    # Linux/GCC/Clang: Enable standard warnings
    target_compile_options(alif-lsp PRIVATE -Wall -Wextra)
endif()