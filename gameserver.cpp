#include <iostream>
#include <thread>
#include <winsock2.h>
#include <vector>
#include <string>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

struct Player {
    SOCKET socket;
    int x, y;  // �������
};

// �������ͻ��˵�����
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
        // ������ҵĶ���
        Player* player = nullptr;
        for (auto& p : players) {
            if (p.socket == client_socket) {
                player = &p;
                break;
            }
        }

        if (player) {
            if (key == 'w') player->y--;  // �����ƶ�
            if (key == 's') player->y++;  // �����ƶ�
            if (key == 'a') player->x--;  // �����ƶ�
            if (key == 'd') player->x++;  // �����ƶ�

            std::string position = "Player at (" + std::to_string(player->x) + ", " + std::to_string(player->y) + ")";
            std::cout << position << std::endl;

            // �㲥λ�ø��¸����пͻ���
            for (auto& p : players) {
                if (p.socket != client_socket) {
                    send(p.socket, position.c_str(), position.size(), 0);
                }
            }
        }
    }
    closesocket(client_socket);
}

// ��������������
int main() {
    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    sockaddr_in server_addr, client_addr;
    int client_size = sizeof(client_addr);
    std::vector<Player> players;

    // ��ʼ�� WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return 1;
    }

    // �����������׽���
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    // ���÷�������ַ
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // ��
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    // ��ʼ����
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

        // ��������
        players.push_back({client_socket, 0, 0});  // ��ʼλ��Ϊ(0,0)

        // Ϊÿ�������Ӵ������߳�
        std::thread(handle_client, client_socket, std::ref(players)).detach();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
