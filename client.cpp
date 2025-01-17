#include <iostream>
#include <string>
#include <winsock2.h>
#include <thread>

using namespace std;
#pragma comment(lib, "ws2_32.lib")  // 需要链接 WS2_32.dll

// 连接到服务端
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

// 发送消息给服务端
void send_message(SOCKET client_socket, const std::string& message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

// 接收并显示服务端消息
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
    std::string server_ip = "10.202.140.85";  // 服务端 IP 地址
    int port = 12345;

    SOCKET client_socket = connect_to_server(server_ip, port);
    if (client_socket == INVALID_SOCKET) return -1;

    // 启动一个线程接收服务端消息
    std::thread(receive_message, client_socket).detach();

    // 向服务端发送消息
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
