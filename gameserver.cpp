#include <iostream>
#include <thread>
#include <winsock2.h>
#include <vector>
#include <string>
#include "gameserver.h"
#include <mutex>
#include <algorithm>
#include <set>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

struct Player {
    SOCKET socket;
    int x, y;       //坐标
    std::string name; //id
};

static int connected_clients = 0;

// 用于保护对connected_clients的访问
static std::mutex client_count_mutex;
static std::mutex name_pool_mutex;

// 名字池，用 set 来自动维持字母序
static std::set<std::string> available_names = {"A", "B", "C", "D"};

std::string getName() {
    std::lock_guard<std::mutex> lock(name_pool_mutex);
    if (!available_names.empty()) {
        std::string newName = *available_names.begin();
        available_names.erase(available_names.begin());
        return newName;
    }
    return ""; // 如果没有可用的名字则返回空串
}

// 释放名字回池中
void releaseName(const std::string& name) {
    std::lock_guard<std::mutex> lock(name_pool_mutex);
    available_names.insert(name);
}

bool updateClientCount(int delta) {
    std::lock_guard<std::mutex> lock(client_count_mutex);
    if (connected_clients + delta >= 0 && connected_clients + delta <= 4) {
        connected_clients += delta;
        std::cout << "Connected clients: " << connected_clients << std::endl;
        return true;
    }
    return false;
}

// 处理单个客户端的连接
void handle_client(SOCKET client_socket, std::vector<Player>& players) {
    char buffer[1];
    int bytes_received;

    std::string player_name = getName();
    if (player_name.empty() || !updateClientCount(1)) {
        std::cerr << "Too many clients or no available names." << std::endl;
        closesocket(client_socket);
        return;
    }

    // 添加新玩家到 players 列表
    Player new_player{client_socket, 0, 0, player_name};
    {
        std::lock_guard<std::mutex> lock(client_count_mutex);
        players.push_back(new_player);
    }

    std::cout << "New player connected as " << player_name << std::endl;

    while (true) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
                std::cerr << "Connection closed or error occurred for " << player_name << "." << std::endl;

                bool success = updateClientCount(-1);
                if (success) {
                    auto it = std::find_if(players.begin(), players.end(),
                                        [client_socket](const Player& p) { return p.socket == client_socket; });
                    if (it != players.end()) {
                        players.erase(it);
                    }
                    releaseName(player_name);
                }

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

            std::string position = "Player "+ player->name +"at (" + std::to_string(player->x) + ", " + std::to_string(player->y) + ")";
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

bool initWinSock(WSADATA &wsaData){
// 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return false;
    }
    return true;
}

bool initServerSocket(SOCKET &server_socket){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return false;
    }
    return true;
}

bool initServer(sockaddr_in &server_addr ,SOCKET &server_socket){
    //设置本地地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        return false;
    }

    // 开始监听
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        return false;
    }
    std::cout << "Server is listening on port "<< SERVER_PORT <<"..." << std::endl;
    return true;
}

// std::string getNextPlayerId() {//目前限制人数为4个
//     static char next_id = 'a' - 1; 
//     if (next_id >= 'a' + 4 - 1) { 
//         return '\0'; 
//     }
//     return "xxx";
// }

// 监听并接受连接
int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    sockaddr_in server_addr, client_addr;
    int client_size = sizeof(client_addr);
    std::vector<Player> players;

    // 初始化 WinSock
    if (!initWinSock(wsaData))
        return 1;

    // 创建服务器套接字
    if(!initServerSocket(server_socket))
        return 1;

    // 初始化本地服务器
    initServer(server_addr,server_socket);

    while (true) {
        SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        // 尝试获取下一个可用的名字
        std::string player_name = getName();
        if (player_name.empty()) {
            std::cerr << "Max player limit reached. Rejecting new connection." << std::endl;
            closesocket(client_socket);
            continue;
        }

        std::cout << "New client connected as " << player_name << std::endl;

        // 如果成功获取了名字，则增加客户端计数并添加新玩家到 players 列表
        if (updateClientCount(1)) {
            players.push_back({client_socket, 0, 0, player_name});  // 初始位置为(0,0)

            // 为每个新连接创建新线程
            std::thread(handle_client, client_socket, std::ref(players)).detach();
        } else {
            // 如果更新客户端计数失败，说明超过了最大连接数
            std::cerr << "Failed to add new client. Max connections reached." << std::endl;
            releaseName(player_name); // 释放刚刚分配的名字
            closesocket(client_socket);
        }
    }

    // while (true) {
    //     client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_size);
    //     if (client_socket == INVALID_SOCKET) {
    //         std::cerr << "Accept failed." << std::endl;
    //         continue;
    //     }

    //     std::cout << "New client connected!" << std::endl;

    //     // 添加新玩家
    //     std::string newPlayerId = getNextPlayerId();
    //     if ("" != newPlayerId) {
    //         players.push_back({client_socket, 0, 0, newPlayerId});  // 初始位置为(0,0)
    //         // 为每个新连接创建新线程
    //         std::thread(handle_client, client_socket, std::ref(players)).detach();
    //     } else {
    //             std::cerr << "Max player limit reached. Rejecting new connection." << std::endl;
    //             closesocket(client_socket);
    //         }
    // }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
