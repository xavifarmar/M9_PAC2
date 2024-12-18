#include <winsock2.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib") // Librería Winsock

void receive_messages(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Se ha perdido la conexión con el servidor.\n";
            closesocket(clientSocket);
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << buffer << std::endl;
    }
}

void send_messages(SOCKET clientSocket) {
    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "/exit") {
            std::cout << "Cerrando la conexión...\n";
            break;
        }

        send(clientSocket, message.c_str(), message.size(), 0);
    }
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error inicializando Winsock.\n";
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error creando el socket.\n";
        WSACleanup();
        return 1;
    }

    std::string serverIP, clientName;
    std::cout << "Introduce la IP del servidor: ";
    std::cin >> serverIP;
    std::cin.ignore(); // Limpiar el buffer del teclado

    std::cout << "Introduce tu nombre: ";
    std::getline(std::cin, clientName);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
    serverAddr.sin_port = htons(12345);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "No se pudo conectar al servidor.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Conectado al servidor.\n";

    // Enviar el nombre del cliente al servidor
    send(clientSocket, clientName.c_str(), clientName.size(), 0);

    // Crear un hilo para recibir mensajes del servidor (y otros clientes)
    std::thread receiverThread(receive_messages, clientSocket);
    receiverThread.detach();  // Detach el hilo para que funcione en segundo plano

    // Crear un hilo para enviar mensajes al servidor
    std::thread senderThread(send_messages, clientSocket);
    senderThread.join();  // Esperar a que el hilo de envío termine

    // Cerrar la conexión
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
