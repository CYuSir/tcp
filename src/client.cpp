#include "client.h"

const int MAX_BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int port = std::stoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect to server: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    char buffer[MAX_BUFFER_SIZE];
    std::string message;
    while (true) {
        std::cout << "Enter message (or 'exit' to quit): ";
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        if (send(sockfd, message.c_str(), message.length(), 0) == -1) {
            std::cerr << "Failed to send message to server: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }
        int n = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
        if (n == -1) {
            std::cerr << "Failed to receive message from server: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        } else if (n == 0) {
            std::cout << "Server closed the connection." << std::endl;
            break;
        } else {
            std::cout << "Received message: " << buffer << std::endl;
        }
    }
    close(sockfd);
    return 0;
}