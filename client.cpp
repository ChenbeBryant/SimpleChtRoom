#include <iostream>
#include <string>
#include <winsock2.h>
#include <thread>

using namespace std;
#pragma comment(lib, "ws2_32.lib")  // ��Ҫ���� WS2_32.dll

// ���ӵ������
SOCKET connect_to_server(const std::string& server_ip, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Connection failed!" << std::endl;
        return INVALID_SOCKET;
    }

    return client_socket;
}

// ������Ϣ�������
void send_message(SOCKET client_socket, const std::string& message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

// ���ղ���ʾ�������Ϣ
void receive_message(SOCKET client_socket) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    std::string server_ip = "10.202.140.85";  // ����� IP ��ַ
    int port = 12345;

    SOCKET client_socket = connect_to_server(server_ip, port);
    if (client_socket == INVALID_SOCKET) return -1;

    // ����һ���߳̽��շ������Ϣ
    std::thread(receive_message, client_socket).detach();

    // �����˷�����Ϣ
    while (true) {
        std::string message;
        std::cout << "Enter message: ";
        std::getline(std::cin, message);
        send_message(client_socket, message);
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
