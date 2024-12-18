#include <winsock2.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <algorithm> // Para std::remove
#include <ws2tcpip.h> // Para inet_ntoa y getaddrinfo

#pragma comment(lib, "ws2_32.lib") // Llibreria Winsock

std::vector<SOCKET> clients;
std::map<SOCKET, std::string> clientNames; // Mapa para guardar nombres de clientes
std::mutex clients_mutex;

void handle_client(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    // Recibir el nombre del cliente
    bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error al recibir el nombre del cliente.\n";
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    std::string clientName(buffer);

    // Asociar el nombre con el socket
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clientNames[clientSocket] = clientName;
    }

    std::cout << clientName << " se ha conectado.\n";

    // Bucle para manejar mensajes
    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << clientNames[clientSocket] << " se ha desconectado.\n";
            closesocket(clientSocket);
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string message = clientNames[clientSocket] + ": " + buffer;
        std::cout << message << std::endl;

        // Retransmitir el mensaje a otros clientes
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (SOCKET client : clients) {
            if (client != clientSocket) {
                send(client, message.c_str(), message.size(), 0);
            }
        }
    }

    // Eliminar el cliente de la lista y del mapa
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
        clientNames.erase(clientSocket);
    }
}

std::string get_server_ip() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "Error obtenint el nom del host.\n";
        return "Desconeguda";
    }

    struct addrinfo hints = {};
    struct addrinfo* result, * ptr;

    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        std::cerr << "Error resolent l'adreça IP del servidor.\n";
        return "Desconeguda";
    }

    // Obtenim la primera adreça IP disponible
    char ip[INET_ADDRSTRLEN];
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)ptr->ai_addr;
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip, sizeof(ip));
        freeaddrinfo(result); // Alliberem els recursos
        return std::string(ip);
    }

    freeaddrinfo(result);
    return "Desconeguda";
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error inicialitzant Winsock.\n";
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creant el socket.\n";
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error en el bind.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error en el listen.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Obtenir la IP del servidor
    std::string server_ip = get_server_ip();
    std::cout << "Servidor en funcionament al port 12345 (IP: " << server_ip << ")...\n";

    while (true) {
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Error acceptant un client.\n";
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(clientSocket);
        }

        std::thread(handle_client, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}