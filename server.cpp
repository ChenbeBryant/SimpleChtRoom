#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <unistd.h>

#pragma comment(lib, "ws2_32.lib")  // 需要链接 WS2_32.dll

std::mutex mtx;  // 保护共享资源
std::vector<SOCKET> clients;  // 保存所有连接的客户端套接字

// 获取当前时间戳
std::string get_timestamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    return std::string(buffer);
}

// 处理客户端消息的线程
void handle_client(SOCKET client_socket) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        // 获取当前时间戳和主机名
        std::string timestamp = get_timestamp();

        char host_name[256]  = "UnknownClient";
        int result;

        result = gethostname(host_name, sizeof(host_name));

        if (result == -1) {
            perror("gethostname");
            return;
        }

        std::string message = timestamp + " " + host_name + ": " + buffer;

        // 将消息转发给所有客户端
        mtx.lock();
        for (SOCKET s : clients) {
            if (s != client_socket) {
                send(s, message.c_str(), message.size(), 0);
            }
        }
        mtx.unlock();
    }
    closesocket(client_socket);
}

// 服务端主程序
void server() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    std::cout << "Server started on port 12345..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        mtx.lock();
        clients.push_back(client_socket);
        mtx.unlock();

        // 启动线程处理该客户端
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
}

int main() {
    server();
    return 0;
}
