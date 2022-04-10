Запуск lobby сервера

g++ ../lobby.cpp -o lobby -lenet && ./lobby

Запуск севера

g++ ../server.cpp -o server -lenet && ./server

Запуск клиента
Для сборки запуска сервера используется следющая команда:

g++ ../client.cpp -o client -lenet && ./client time
time то через сколько после запуска будет отправлено сообщение старт лобби-серверу