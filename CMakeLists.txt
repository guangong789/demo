cmake_minimum_required(VERSION 3.28)
project(NETWORK LANGUAGES CXX DESCRIPTION "A C++ network application")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)
find_package(Threads REQUIRED)

add_library(gaozu_utility STATIC
    src/utility/logger.cpp
    src/utility/epoll.cpp
    src/utility/connct.cpp
    src/utility/threadpool.cpp
    src/utility/conn_manager.cpp
)
target_include_directories(gaozu_utility PUBLIC include)
target_link_libraries(gaozu_utility PUBLIC Threads::Threads)

add_library(gaozu_socket STATIC
    src/socket/socket.cpp
    src/socket/server_socket.cpp
    src/socket/client_socket.cpp
)
target_include_directories(gaozu_socket PUBLIC include)
target_link_libraries(gaozu_socket PUBLIC Threads::Threads)

add_library(gaozu_http STATIC
    src/http/request.cpp
    src/http/response.cpp
    src/http/router.cpp
)
target_include_directories(gaozu_http PUBLIC include)
target_link_libraries(gaozu_http PUBLIC gaozu_utility)

add_executable(http_test src/test/http_test.cpp)
target_include_directories(http_test PRIVATE include)
target_link_libraries(http_test PRIVATE gaozu_http gaozu_utility)

add_executable(server src/applications/server.cpp)
target_link_libraries(server PRIVATE gaozu_socket gaozu_utility gaozu_http)

add_executable(client src/applications/client.cpp)
target_link_libraries(client PRIVATE gaozu_socket gaozu_utility)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

install(TARGETS server client RUNTIME DESTINATION bin)
install(TARGETS gaozu_socket gaozu_utility gaozu_http ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
if(EXISTS "${CMAKE_SOURCE_DIR}/config")
    install(DIRECTORY config/ DESTINATION config)
endif()

message(STATUS "=== NETWORK Project Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Executables: server, client, http_test")
