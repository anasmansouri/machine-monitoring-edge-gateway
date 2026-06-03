#pragma once

#include <string>

class IpcClient {
public:
    explicit IpcClient(const std::string& socketPath);
    ~IpcClient();

    bool connectToServer();
    std::string readLine();

private:
    std::string socketPath_;
    int socketFd_;
};
