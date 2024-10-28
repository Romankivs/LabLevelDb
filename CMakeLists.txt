cmake_minimum_required(VERSION 3.10)
project(LevelDBProject)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch LevelDB
FetchContent_Declare(
    leveldb
    GIT_REPOSITORY https://github.com/google/leveldb.git
    GIT_TAG        main
)

# Fetch nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        develop
)

# Fetch fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        master
)

FetchContent_MakeAvailable(leveldb nlohmann_json fmt)

add_executable(leveldb_app main.cpp)

target_link_libraries(leveldb_app leveldb nlohmann_json::nlohmann_json fmt)
