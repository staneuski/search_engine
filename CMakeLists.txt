cmake_minimum_required(VERSION 3.0.0)
project(search_engine VERSION 0.4.3)

set(CMAKE_CXX_STANDARD 17)

include(CTest)
enable_testing()

add_executable(main src/main.cpp
                    src/request_queue.cpp
                    src/search_server.cpp
                    src/string_processing.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
