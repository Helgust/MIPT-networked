Сборка файлов

g++ ./server.cpp -o ./bin/server -lenet
g++ ./lobby.cpp -o ./bin/lobby -lenet
g++ ./client.cpp -o ./bin/client -lenet

Примерный сценарий работы
Запустить ./lobby
Запустить ./server
Запустить ./client
В клиенте присодениться через /join <номер комнаты>
В лобби запусить комнату