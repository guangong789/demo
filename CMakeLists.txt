cmake_minimum_required(VERSION 3.20)
project(NETWORK
    LANGUAGES CXX
    DESCRIPTION "A C++ network application"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译器警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 包含目录
include_directories(include)

# Socket 网络库
add_library(gaozu_socket STATIC
    src/socket/socket.cpp
    src/socket/server_socket.cpp
    src/socket/client_socket.cpp
)

# Utility 工具库
add_library(gaozu_utility STATIC
    src/utility/logger.cpp
    src/utility/epoll.cpp
    src/utility/connct.cpp
)

# 设置库的包含目录
target_include_directories(gaozu_socket PUBLIC include)
target_include_directories(gaozu_utility PUBLIC include)

# 查找并链接线程库
find_package(Threads REQUIRED)
target_link_libraries(gaozu_socket Threads::Threads)

# Utility 库可能也需要线程库（如果 logger 或 epoll 使用了线程）
target_link_libraries(gaozu_utility Threads::Threads)

# 可执行文件 - 服务端
add_executable(server
    src/applications/server.cpp
)
target_link_libraries(server gaozu_socket gaozu_utility)

# 可执行文件 - 客户端
add_executable(client
    src/applications/client.cpp
)
target_link_libraries(client gaozu_socket gaozu_utility)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 安装目标
install(TARGETS server client
    RUNTIME DESTINATION bin
)

install(TARGETS gaozu_socket gaozu_utility
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# 安装头文件
install(DIRECTORY include/ DESTINATION include)

# 安装配置文件（如果存在 config 目录）
if(EXISTS "${CMAKE_SOURCE_DIR}/config")
    install(DIRECTORY config/ DESTINATION config)
endif()

# 输出信息
message(STATUS "=== NETWORK Project Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Libraries: gaozu_socket, gaozu_utility")
message(STATUS "Executables: server, client")
message(STATUS "Source Structure:")
message(STATUS "  - Socket Library: src/socket/")
message(STATUS "  - Utility Library: src/utility/")
message(STATUS "  - Applications: src/applications/")
message(STATUS "  - Headers: include/")