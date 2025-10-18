cmake_minimum_required(VERSION 3.29)
project(List)

set(CMAKE_CXX_STANDARD 20)

add_executable(List List.cpp
        net/TcpSocket.cpp
        net/TcpSocket.h
        net/TcpClient.cpp
        net/TcpClient.h
        Queue.cpp
        Queue.h
        Server.cpp
        Server.h
        common/config.h
        MessageDispatcher.cpp
        MessageDispatcher.h
        common/Logger.h
        Logger.cpp
        BroadcastDispatcher.cpp
        BroadcastDispatcher.h)
add_executable(Consumer Consumer.cpp
        time_stamp.h
        timeStamp.cpp
        InitClientSocket.h
        InitClientSocket.cpp
        mythreadpool.h
        net/TcpSocket.cpp
        net/TcpSocket.h
        net/TcpBase.cpp
        net/TcpBase.h
        net/TcpServer.cpp
        net/TcpServer.h
        net/TcpClient.cpp
        net/TcpClient.h
        ConsumerManager.cpp
        ConsumerManager.h
Logger.cpp
common/Logger.h)
add_executable(Producer Producer.cpp
        time_stamp.h
        timeStamp.cpp
        InitClientSocket.cpp
        net/TcpSocket.cpp
        net/TcpSocket.h
        net/TcpClient.cpp
        net/TcpClient.h)
add_executable(find findrepeat.cpp)
add_executable(test testServer.cpp
        Server.h
        Server.cpp
        Receiver.cpp
        Logger.cpp
        persistence.cpp
        persistence.h
        ClusterDispatcher.cpp
        ClusterDispatcher.h
        ConsumerManager.h
        ConsumerManager.cpp
)

target_link_libraries(List PRIVATE ws2_32)
target_link_libraries(Consumer PRIVATE ws2_32)
target_link_libraries(Producer PRIVATE ws2_32)
target_link_libraries(test PRIVATE ws2_32)