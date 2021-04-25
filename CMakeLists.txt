cmake_minimum_required(VERSION 3.0.0)
project(search_engine VERSION 0.6.0)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror)

include_directories(src tests)
set(SRC
    src/process_queries.cpp
    src/remove_duplicates.cpp
    src/request_queue.cpp
    src/search_server.cpp
    src/string_processing.cpp)

#######################################
# SRC
#######################################
add_executable(main src/main.cpp ${SRC})


#######################################
# TESTS
#######################################
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/0d9daa99365eb00a2b81392b6ec2c8a802769ef0.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(gtest-search_server tests/gtest-search_server.cpp ${SRC})
target_link_libraries(gtest-search_server gtest_main)
add_test(NAME search_server COMMAND gtest-search_server)


#######################################
# BENCHMARKS
#######################################
add_executable(benchmark-search_server tests/benchmark-search_server.cpp ${SRC})
add_executable(benchmark-string_processing tests/benchmark-string_processing.cpp ${SRC})
