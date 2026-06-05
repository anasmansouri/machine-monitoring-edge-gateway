#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <functional>

namespace cc::manager {

class IpcServer {
public:
    // Define a clean type alias for the callback
    using MessageCallback = std::function<void(int clientFd, const std::string& message)>;
    explicit IpcServer(std::string socketPath,MessageCallback callback=nullptr);
    ~IpcServer();

    bool start();
    void stop();

    bool broadcastLine(const std::string& line);
    bool sendMsg(int clientFd,const std::string& line);

private:
    void acceptLoop();
    void readLoop(int clientFd);
    void closeClient(int clientFd);

    std::string socketPath_;

    int serverFd_{-1};

    std::atomic<bool> running_{false};

    std::thread acceptThread_;

    std::mutex clientsMutex_;
    std::vector<int> clients_;
    MessageCallback onMessageReceived_;
};

}
