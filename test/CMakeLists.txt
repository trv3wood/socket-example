set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/release)
add_executable(testpool 
    testpool.cpp 
    ${CMAKE_SOURCE_DIR}/src/threadpool.cpp 
    ${CMAKE_SOURCE_DIR}/src/threadpool.h)
target_include_directories(testpool 
    PRIVATE ${CMAKE_SOURCE_DIR}/src)