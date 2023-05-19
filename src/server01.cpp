#include "server.h"

int create_non_blocking_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void handle_new_connection(int listen_fd, int epoll_fd) {
    std::cout << __func__ << " " << __LINE__ << std::endl;
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        std::cerr << "Failed to accept new connection: " << strerror(errno) << std::endl;
        return;
    }
    set_non_blocking(client_fd);
    epoll_event ev;
    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        std::cerr << "Failed to add client socket to epoll: " << strerror(errno) << std::endl;
        close(client_fd);
    }
    client_active_time[client_fd] = time(nullptr);
}

void handle_client_message(int client_fd, int epoll_fd) {
    // std::cout << __func__ << " " << __LINE__ << std::endl;
    char buffer[MAX_BUFFER_SIZE] = {0};
    int n;
    while ((n = recv(client_fd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        // memset(buffer, 0, MAX_BUFFER_SIZE);
        // stpcpy(buffer, buffer, n);
        std::cout << "Received message: " << buffer << ", n = " << n << std::endl;
        send(client_fd, buffer, n, 0);
        std::cout << "Sent message: " << buffer << ", n = " << n << std::endl;
    }
    std::cout << "n = " << n << ", errno = " << errno << std::endl;
    if (n == -1 && errno != EAGAIN) {
        std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
    }
    // close(client_fd);
    client_active_time[client_fd] = time(nullptr);
}

void close_inactive_clients(int epoll_fd, std::unordered_map<int, time_t>& client_active_time, int timeout_sec) {
    time_t now = time(nullptr);
    for (auto it = client_active_time.begin(); it != client_active_time.end();) {
        if (now - it->second > timeout_sec) {
            // Remove client from epoll
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, nullptr);

            // Close client connection
            close(it->first);
            std::cout << "Client " << it->first << " disconnected due to inactivity." << std::endl;

            // Remove client from the map
            it = client_active_time.erase(it);
        } else {
            ++it;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        exit(EXIT_FAILURE);
    }
    int check_interval = 1;  // Check every 10 iterations
    int counter = 0;
    // int port = atoi(argv[1]);
    int port = std::stoi(argv[1]);
    int listen_fd = create_non_blocking_socket();
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        std::cerr << "Failed to add listening socket to epoll: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::vector<epoll_event> events(MAX_EVENTS);
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, TIMEOUT_MS);
        if (nfds == -1) {
            std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        // Check for inactive clients
        if (++counter >= check_interval) {
            close_inactive_clients(epoll_fd, client_active_time, TIMEOUT_MS / 1000);
            counter = 0;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listen_fd) {
                handle_new_connection(listen_fd, epoll_fd);
            } else {
                std::cout << "fd = " << events[i].data.fd << std::endl;
                handle_client_message(events[i].data.fd, epoll_fd);
            }
        }
    }
    close(listen_fd);
    close(epoll_fd);
    return 0;
}