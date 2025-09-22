//
// Created by 13717 on 2025/4/20.
//

#ifndef LIST_INITCLIENTSOCKET_H
#define LIST_INITCLIENTSOCKET_H
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
int InitClientSocket(SOCKET &serverSocket, const char *ip, int port);
#endif //LIST_INITCLIENTSOCKET_H
