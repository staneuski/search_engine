cmake_minimum_required(VERSION 3.0.0)
project(search_engine VERSION 0.5.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Werror)

set(SRC
    src/remove_duplicates.cpp
    src/request_queue.cpp
    src/search_server.cpp
    src/string_processing.cpp)


#######################################
# GOOGLE TESTS
#######################################
find_package(GTest REQUIRED)
include_directories(test src GTest::Main ${GTEST_INCLUDE_DIRS})
add_executable(gtests test/main.cpp ${SRC})


#######################################
# UNIT TESTS
#######################################
enable_testing()
target_link_libraries(gtests ${GTEST_LIBRARIES})
add_test(NAME search_server
         COMMAND gtests)


#######################################
# MAIN
#######################################
add_executable(main src/main.cpp ${SRC})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})