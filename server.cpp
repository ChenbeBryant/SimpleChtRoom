#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <unistd.h>

#pragma comment(lib, "ws2_32.lib")  // ��Ҫ���� WS2_32.dll

std::mutex mtx;  // ����������Դ
std::vector<SOCKET> clients;  // �����������ӵĿͻ����׽���

// ��ȡ��ǰʱ���
std::string get_timestamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    return std::string(buffer);
}

// ����ͻ�����Ϣ���߳�
void handle_client(SOCKET client_socket) {
    char buffer[1024];
    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        // ��ȡ��ǰʱ�����������
        std::string timestamp = get_timestamp();

        char host_name[256]  = "UnknownClient";
        int result;

        result = gethostname(host_name, sizeof(host_name));

        if (result == -1) {
            perror("gethostname");
            return;
        }

        std::string message = timestamp + " " + host_name + ": " + buffer;

        // ����Ϣת�������пͻ���
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

// �����������
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

        // �����̴߳���ÿͻ���
        std::thread(handle_client, client_socket).detach();
    }

    closesocket(server_socket);
    WSACleanup();
}

int main() {
    server();
    return 0;
}
