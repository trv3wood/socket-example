enable_testing()
include(CTest)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/release)

add_executable(testpool 
    testpool.cpp 
    ${CMAKE_SOURCE_DIR}/src/threadpool.cpp 
    ${CMAKE_SOURCE_DIR}/src/threadpool.h)
target_include_directories(testpool 
    PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME testpool
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testpool)

add_executable(testserver 
    testserver.cpp)
target_link_libraries(testserver 
    PRIVATE server)
target_include_directories(testserver 
    PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME testserver
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testsever)

add_executable(testbywrt 
    testbywrt.cpp)
add_test(NAME testbywrt
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/testbywrt)