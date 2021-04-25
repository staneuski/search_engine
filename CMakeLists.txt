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
#- Google tests
find_package(GTest REQUIRED)
include_directories(tests src ${GTEST_INCLUDE_DIRS})
add_executable(gtest-search_server tests/gtest-search_server.cpp ${SRC})

#- Run gtests using cmake testing
enable_testing()
target_link_libraries(gtest-search_server GTest::Main ${GTEST_LIBRARIES})
add_test(NAME search_server COMMAND gtest-search_server)


#######################################
# BENCHMARKS
#######################################
add_executable(benchmark-search_server tests/benchmark-search_server.cpp ${SRC})
add_executable(benchmark-string_processing tests/benchmark-string_processing.cpp ${SRC})
