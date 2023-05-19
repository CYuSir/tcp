#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;
const int MAX_EVENTS = 2;
const int TIMEOUT_MS = 10000;
const int MAX_BUFFER_SIZE = 1024;

std::unordered_map<int, time_t> client_active_time;

int create_non_blocking_socket();

void set_non_blocking(int sockfd);

void handle_new_connection(int listen_fd, int epoll_fd);

void handle_client_message(int client_fd);

void close_inactive_clients(int epoll_fd, std::unordered_map<int, time_t>& client_active_time, int timeout_sec);