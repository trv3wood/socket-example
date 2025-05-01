cmake_minimum_required(VERSION 3.10)
project(socket VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/release)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
file(GLOB SRC src/*.cpp src/*.h)
add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)

enable_testing()
add_subdirectory(test)
add_test(NAME testpool
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/testpool)