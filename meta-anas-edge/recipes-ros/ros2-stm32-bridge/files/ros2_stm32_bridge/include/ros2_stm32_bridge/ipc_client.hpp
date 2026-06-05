#pragma once

#include <string>

class IpcClient {
public:
    explicit IpcClient(const std::string& socketPath);
    ~IpcClient();

    bool connectToServer();
    std::string readLine();
    bool writeLine(const std::string& line);

private:
    std::string socketPath_;
    int socketFd_;
};
