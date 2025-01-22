;g++ -o client ./client.cpp -lws2_32
;g++ -o server ./server.cpp -lws2_32

g++ -o server.exe .\gameserver.h .\gameserver.cpp -lws2_32
g++ -o client.exe .\gameserver.h .\gameclient.cpp -lws2_32