cmake_minimum_required(VERSION 3.15)
project(trading_processor CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置默认构建类型为 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
if(MSVC)
    # MSVC 编译选项
    add_compile_options(/W4 /WX)
    # Debug版本
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
    # Release版本
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")
else()
    # GCC/Clang 编译选项
    add_compile_options(-Wall -Wextra)
    # Debug版本
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    # Release版本
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/io_utils.cpp
    src/binance_fetcher.cpp
    src/output_handler.cpp
    src/processor.cpp
    src/footprint.cpp
)

# 头文件
set(HEADERS
    include/config.h
    include/trade.h
    include/data_fetcher.h
    include/io_utils.h
    include/output_handler.h
    include/processor.h
    include/footprint.h
    include/json.hpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
