#include "server.h"

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

// #include "thread.h"

using namespace std;
// const int MAX_EVENTS = 10;
const int BUFFER_SIZE = 1024;
ThreadPool threadPool(4);

void handleMessage(int client_socket, const char* buf, int bytesRead) {
#if 1
    cout << "Received: " << string(buf, bytesRead) << endl;
    std::string replyMessage = "Server: " + string(buf, bytesRead);
    cout << "Sending: " << replyMessage << endl;
    send(client_socket, replyMessage.c_str(), replyMessage.size(), 0);
#else
    threadPool.addTask(client_socket, buf, bytesRead);
#endif
}

int eventLoop() {
    int serverSocket, epollFd;
    struct sockaddr_in serverAddr;

    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return -1;
    }

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket option." << std::endl;
        return -1;
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Failed to bind." << endl;
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) == -1) {
        cerr << "Failed to listen." << endl;
        close(serverSocket);
        return -1;
    }

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        cerr << "Failed to create epoll instance." << endl;
        close(serverSocket);
        return -1;
    }

    struct epoll_event event;
    event.data.fd = serverSocket;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1) {
        cerr << "Failed to add socket to epoll." << endl;
        close(serverSocket);
        close(epollFd);
        return -1;
    }
    struct epoll_event events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];
    while (true) {
        int readyFdCount = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        if (readyFdCount == -1) {
            cerr << "epoll_wait error" << endl;
            continue;
        }
        for (int i = 0; i < readyFdCount; ++i) {
            int currentSocket = events[i].data.fd;
            if (currentSocket == serverSocket) {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (clientSocket == -1) {
                    cerr << "accept error" << endl;
                    continue;
                }

                int flags = fcntl(clientSocket, F_GETFL, 0);
                fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

                struct epoll_event event;
                event.data.fd = clientSocket;
                event.events = EPOLLIN | EPOLLET;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
                    cerr << "epoll_ctl error" << endl;
                    close(clientSocket);
                    continue;
                }
                cout << "New client connected"
                     << ", new socket: " << clientSocket << endl;
            } else {
                int bytesRead = recv(currentSocket, buffer, BUFFER_SIZE, 0);
                // 在eventLoop函数中处理客户端关闭连接的情况
                if (bytesRead == 0) {
                    std::cout << "Client disconnected" << std::endl;
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, currentSocket, nullptr);  // 从epoll实例中移除套接字
                    close(currentSocket);
                    continue;
                }
                // handleMessage(currentSocket, buffer, bytesRead);
                auto task = [currentSocket, buffer, bytesRead]() { handleMessage(currentSocket, buffer, bytesRead); };
                threadPool.enqueue(task);
            }
        }
    }

    close(serverSocket);
    close(epollFd);
    return 0;
}

int main(const int argc, const char* argv[]) {
    std::thread eventLoopThread(eventLoop);

    eventLoopThread.join();

    return 0;
}