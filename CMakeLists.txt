cmake_minimum_required(VERSION 3.20)
project(TCPServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json for IDE/LSP (clangd, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(TCPServer
    src/main.cpp
    src/network.cpp
    src/auth.cpp
    src/database.cpp
    src/game.cpp
    src/logger.cpp
    src/protocol.cpp
)

target_include_directories(TCPServer PRIVATE src)

target_compile_options(TCPServer PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
