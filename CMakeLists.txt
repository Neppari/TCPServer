cmake_minimum_required(VERSION 3.20)
project(TCPServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sodium REQUIRED IMPORTED_TARGET libsodium)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# --- Server ---
add_executable(TCPServer
    src/main.cpp
    src/network.cpp
    src/protocol.cpp
    src/auth.cpp
    src/database.cpp
    src/game.cpp
    src/logger.cpp
)
target_include_directories(TCPServer PRIVATE src)
target_link_libraries(TCPServer PRIVATE
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
    Threads::Threads
    PkgConfig::sodium
)
target_compile_options(TCPServer PRIVATE -Wall -Wextra -Wpedantic)

# --- Good Client ---
add_executable(GoodClient
    clients/good_client.cpp
    src/protocol.cpp
)
target_include_directories(GoodClient PRIVATE src)
target_link_libraries(GoodClient PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(GoodClient PRIVATE -Wall -Wextra -Wpedantic)

# --- Malicious Client ---
add_executable(MaliciousClient
    clients/malicious_client.cpp
    src/protocol.cpp
)
target_include_directories(MaliciousClient PRIVATE src)
target_link_libraries(MaliciousClient PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(MaliciousClient PRIVATE -Wall -Wextra -Wpedantic)
