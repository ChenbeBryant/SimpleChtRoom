#include <iostream>
#include <thread>
#include <winsock2.h>
#include <vector>
#include <string>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

struct Player {
    SOCKET socket;
    int x, y;  // 玩家坐标
};

// 处理单个客户端的连接
void handle_client(SOCKET client_socket, std::vector<Player>& players) {
    char buffer[1];
    int bytes_received;

    while (true) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Connection closed or error occurred." << std::endl;
            break;
        }

        char key = buffer[0];
        // 处理玩家的动作
        Player* player = nullptr;
        for (auto& p : players) {
            if (p.socket == client_socket) {
                player = &p;
                break;
            }
        }

        if (player) {
            if (key == 'w') player->y--;  // 向上移动
            if (key == 's') player->y++;  // 向下移动
            if (key == 'a') player->x--;  // 向左移动
            if (key == 'd') player->x++;  // 向右移动

            std::string position = "Player at (" + std::to_string(player->x) + ", " + std::to_string(player->y) + ")";
            std::cout << position << std::endl;

            // 广播位置更新给所有客户端
            for (auto& p : players) {
                if (p.socket != client_socket) {
                    send(p.socket, position.c_str(), position.size(), 0);
                }
            }
        }
    }
    closesocket(client_socket);
}

// 监听并接受连接
int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    sockaddr_in server_addr, client_addr;
    int client_size = sizeof(client_addr);
    std::vector<Player> players;

    // 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return 1;
    }

    // 创建服务器套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    // 开始监听
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        return 1;
    }

    std::cout << "Server is listening on port 12345..." << std::endl;

    while (true) {
        client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        std::cout << "New client connected!" << std::endl;

        // 添加新玩家
        players.push_back({client_socket, 0, 0});  // 初始位置为(0,0)

        // 为每个新连接创建新线程
        std::thread(handle_client, client_socket, std::ref(players)).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
