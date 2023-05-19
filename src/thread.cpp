#include "thread.h"

ThreadPool::ThreadPool(int numThreads) : _numThreads(numThreads), stop(false) {
    for (int i = 0; i < _numThreads; ++i) {
        threads.emplace_back(thread(&ThreadPool::workerThread, this));
    }
}

ThreadPool::~ThreadPool() {
    unique_lock<mutex> lock(taskMutex);

    stop = true;

    taskCV.notify_all();

    for (auto& t : threads) {
        t.join();
    }
}

void ThreadPool::addTask(int client_socket, const char* buf, int bytesRead) {
    std::string bufData(buf, bytesRead);
    unique_lock<mutex> lock(taskMutex);
    tasks.emplace(Task{client_socket, bytesRead, bufData});
    taskCV.notify_one();
}

void ThreadPool::workerThread() {
    while (true) {
        Task task;
        {
            unique_lock<mutex> lock(taskMutex);
            taskCV.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) {
                return;
            }
            task = tasks.front();
            tasks.pop();
        }
        processTask(task);
    }
}

void ThreadPool::processTask(const Task& task) {
    this_thread::sleep_for(chrono::seconds(1));
    std::cout << "Processing task " << task.client_socket << std::endl;
    std::cout << "Bytes read: " << task.bytesRead << std::endl;
    std::cout << "Buffer: " << task.bufData << std::endl;  // 使用 bufData 成员变量获取字符串内容
    std::string replyMessage = "Server: " + task.bufData + "\n";
    std::cout << "Sending: " << replyMessage << std::endl;
    send(task.client_socket, replyMessage.c_str(), replyMessage.size(), 0);
    std::cout << "Done processing task " << task.client_socket << std::endl;
}