cmake_minimum_required(VERSION 3.29)
project(List)

set(CMAKE_CXX_STANDARD 20)

add_executable(List List.cpp)
add_executable(Consumer Consumer.cpp
        time_stamp.h
        timeStamp.cpp
        InitClientSocket.h
        InitClientSocket.cpp
        mythreadpool.h)
add_executable(Producer Producer.cpp
        time_stamp.h
        timeStamp.cpp
        InitClientSocket.cpp)
add_executable(find findrepeat.cpp)

target_link_libraries(List PRIVATE ws2_32)
target_link_libraries(Consumer PRIVATE ws2_32)
target_link_libraries(Producer PRIVATE ws2_32)