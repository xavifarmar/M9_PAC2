#include <iostream>
#include <winsock2.h>

int main() {
    // Inicialitza la llibreria Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0) {
        std::cerr << "Error inicialitzant Winsock2. Codi d'error: " << result << std::endl;
        return 1;
    }

    std::cout << "Sockets configurats correctament" << std::endl;

    // Finalitza la llibreria Winsock quan ja no es necessita
    WSACleanup();

    return 0;
}