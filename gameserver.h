 #pragma once 

#include <iostream>
#include <thread>
#include <winsock2.h>
#include <vector>
#include <string>
#include <set>
#include <cstring>
#include <mutex>
#include <algorithm>
#include <set>

#define SERVER_ADDR "10.202.140.85"
#define SERVER_PORT 12345

struct Player {
    SOCKET socket;
    int x, y;       //зјБъ
    std::string name; //id
};
struct PacketPlayerInfo
{
	int hp;
	int score;
	char name[32]; 
	int x, y;
	

    PacketPlayerInfo(int _hp, int _score, const std::string& _name, int _x, int _y)
        : hp(_hp), score(_score), x(_x), y(_y) {
        strncpy(name, _name.c_str(), sizeof(name)-1);
        name[sizeof(name)-1] = '\0';
    }
};