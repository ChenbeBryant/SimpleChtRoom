#include <iostream>
#include <thread>
#include <conio.h>  // 用于_getch()，可以替代标准输入
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// 用于发送消息给服务端
void send_key_to_server(SOCKET server_socket) {
    while (true) {
        // 检查是否有键盘输入
        if (_kbhit()) {
            char key = _getch();  // 获取按键
            if (key == 'q') {
                break;  // 'q' 按下时退出程序
            }

            // 发送按键给服务端
            send(server_socket, &key, 1, 0);
            std::cout << "Sent key: " << key << std::endl;
        }
    }
}

int main() {
    WSADATA wsaData;
    SOCKET server_socket;
    sockaddr_in server_addr;

    // 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return 1;
    }

    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接到服务端
    if (connect(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    // 创建线程来发送按键数据
    std::thread sender_thread(send_key_to_server, server_socket);
    sender_thread.join();

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
