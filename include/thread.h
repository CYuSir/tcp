#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std;

const int NUM_THREADS = 4;  // 线程池中的线程数量

struct Task {
    int client_socket;
    int bytesRead;
    std::string bufData;
};

class ThreadPool {
   public:
    ThreadPool(int numThreads);
    ~ThreadPool();
    // void addTask(int client_socket, char* buf, int bytesRead);
    void addTask(int client_socket, const char* buf, int bytesRead);

   private:
    void workerThread();
    void processTask(const Task& task);

    int _numThreads;
    bool stop;
    vector<thread> threads;
    queue<Task> tasks;
    mutex taskMutex;
    condition_variable taskCV;
};