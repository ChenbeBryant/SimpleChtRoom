
#include "gameserver.h"
#include <conio.h>  // 用于_getch()，可以替代标准输入

#pragma comment(lib, "ws2_32.lib")

bool initClient(WSADATA &wsaData, SOCKET &server_socket, sockaddr_in &server_addr){
    // 初始化 WinSock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed." << std::endl;
        return false;
    }

    // 创建套接字
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket." << std::endl;
        return false;
    }

    // 设置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    // 连接到服务端
    if (connect(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed." << std::endl;
        return false;
    }
    return true;
}

void sendMsgToServer(SOCKET server_socket) {
    while (true) {
        // 检查是否有键盘输入
        if (_kbhit()) {
            char key = _getch();  // 获取按键
            if (key == 'q') {
                break;  // 'q' 按下时退出程序
            }

            // 发送按键给服务端
            send(server_socket, &key, 1, 0);
            //std::cout << "Sent key: " << key << std::endl;
        }
    }
}

// 定义一个互斥锁来保护共享资源
std::mutex playerMutex;

// 全局变量存储所有玩家信息
std::vector<PacketPlayerInfo> players;

//本客户端playername
std::string my_playername;

#include <array>

// 定义一个40x40的字符网格，用于表示当前的显示状态
std::array<std::array<char, 40>, 40> displayGrid;
std::string displayBuffer; // 用于一次性输出整个屏幕内容

void initializeDisplay() {
    // 初始化displayGrid为空格
    for (auto& row : displayGrid) {
        row.fill(' ');
    }
}

void updateDisplay() {
    // 保护对players的访问
    std::lock_guard<std::mutex> lock(playerMutex);

    // 清屏并设置光标位置
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        COORD homeCoord = { 0, 0 };
        DWORD written;
        FillConsoleOutputCharacter(hConsole, ' ', 40 * 40 + 1, homeCoord, &written);
        SetConsoleCursorPosition(hConsole, homeCoord);
    #else
        printf("\033[2J\033[H"); // ANSI escape sequence to clear screen and move cursor to top-left
    #endif

    // 打印HP和Score
    auto localPlayerIt = std::find_if(players.begin(), players.end(),
                                      [](const PacketPlayerInfo& player ) -> bool {
                                            return (strcmp(player.name, my_playername.c_str()) == 0);
                                      });

    if (localPlayerIt != players.end()) {
        std::cout <<"Player: " <<my_playername<<" HP: " << localPlayerIt->hp << " Score: " << localPlayerIt->score << " Position: "<< localPlayerIt->x <<","<<localPlayerIt->y<<"\n";
    } else {
        std::cout << "HP: N/A Score: N/A\n"; // 如果找不到本地玩家，打印N/A
    }


    // 更新displayGrid中的玩家位置
    for (const auto& player : players) {
        if (player.x >= 0 && player.x < 40 && player.y >= 0 && player.y < 40) {
            displayGrid[player.y][player.x] = player.name[0];
        }
    }

    // 构建整个屏幕的字符串表示
    displayBuffer.clear();
    for (int y = 39; y >= 0; --y) {
        for (int x = 0; x < 40; ++x) {
            displayBuffer.push_back(displayGrid[y][x]);
        }
        displayBuffer.push_back('\n');
    }

    // 一次性输出整个屏幕内容
    std::cout << displayBuffer;

    // 重置displayGrid为下一个周期做准备
    initializeDisplay();
}
void receiveMessage(SOCKET client_socket) {
    // 定义一个 PacketPlayerInfo 类型的缓冲区
    PacketPlayerInfo playerInfo{0,0,"",0,0};
    
    while (true) {
        // 接收数据，确保我们只接收 sizeof(PacketPlayerInfo) 个字节
        int bytes_received = recv(client_socket, reinterpret_cast<char*>(&playerInfo), sizeof(PacketPlayerInfo), 0);
        
        if (bytes_received <= 0) {
            // 如果接收到的数据长度不大于0，表示连接关闭或发生错误
            std::cerr << "Connection closed or error occurred." << std::endl;
            break;
        }

        // 此处假设所有整数字段已经在发送前被转换为网络字节序（大端），如果需要的话，现在应该转回主机字节序。
        // 使用 ntohl 或 ntohs 函数对需要转换的整数字段进行转换。
        // 注意：只有程序可能在不同字节序的系统间通信时才需要这样做。
        // 这里以 hp 和 score 字段为例：
        // playerInfo.hp = ntohl(playerInfo.hp);
        // playerInfo.score = ntohl(playerInfo.score);
        // playerInfo.x = ntohl(playerInfo.x);
        // playerInfo.y = ntohl(playerInfo.y);

        // 打印接收到的信息
        // std::cout << "Received Player Info: HP=" << playerInfo.hp 
        //           << ", Score=" << playerInfo.score 
        //           << ", Name=" << playerInfo.name 
        //           << ", Position=(" << playerInfo.x 
        //           << ", " << playerInfo.y << ")" << std::endl;

        // 更新玩家信息
        {
            std::lock_guard<std::mutex> lock(playerMutex);

            bool found = false;
            for (auto& player : players) {
                if (strcmp(player.name, playerInfo.name) == 0) {
                    player.hp = playerInfo.hp;  
                    player.score = playerInfo.score;
                    player.x = playerInfo.x;
                    player.y = playerInfo.y;
                    found = true;
                    break;
                }
            }
            if (!found) {
                players.push_back(playerInfo);
            }
        }
        updateDisplay();
        //todo 这里可以添加更多代码来处理接收到的玩家信息

        // 注意：在这个简单的例子中，我们假设每个recv调用正好接收到一个完整的PacketPlayerInfo对象。
        // 日后可能需要处理部分读取的情况，即一次recv调用可能只接收到部分数据，
        // 并且需要实现逻辑来累积这些部分到拥有完整的PacketPlayerInfo对象。
    }
}


int main() {
    WSADATA wsaData;
    SOCKET server_socket;
    sockaddr_in server_addr;

    if (initClient(wsaData, server_socket, server_addr))
        std::cout << "Connected to server." << std::endl;

    // 接收玩家名字
    char playerNameBuffer[128]; // 定义一个足够大的缓冲区来接收名字
    int bytes_received = recv(server_socket, playerNameBuffer, sizeof(playerNameBuffer) - 1, 0);
    if (bytes_received > 0) {
        playerNameBuffer[bytes_received] = '\0'; // 确保字符串以空字符结尾
        my_playername = playerNameBuffer; // 设置为全局变量
        std::cout << "Your player name is: " << my_playername << std::endl;
    } else {
        std::cerr << "Failed to receive player name from server." << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    // 启动接收和发送线程，并将它们存储在一个vector中
    std::vector<std::thread> threads;
    threads.emplace_back(receiveMessage, server_socket);
    threads.emplace_back(sendMsgToServer, server_socket);

    // 主线程等待所有工作线程完成
    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
