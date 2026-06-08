#include "ros2_stm32_bridge/ipc_client.hpp"

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

IpcClient::IpcClient(const std::string& socketPath)
    : socketPath_(socketPath),
      socketFd_(-1)
{
}

IpcClient::~IpcClient()
{
    this->disconnectFromServer();
}

bool IpcClient::connectToServer()
{
    this->disconnectFromServer();
    socketFd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socketFd_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    std::strncpy(
        addr.sun_path,
        this->socketPath_.c_str(),
        sizeof(addr.sun_path) - 1);

    if (connect(
            socketFd_,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) < 0) {
        perror("connect");
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }

    return true;
}

std::string IpcClient::readLine()
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    ssize_t bytesRead =
        read(socketFd_, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        return std::string(buffer);
    }
    this->disconnectFromServer();
    return "";
}
bool IpcClient::writeLine(const std::string& line){
    if (socketFd_ < 0) {
        std::cerr << "Client not connected, cannot write." << std::endl;
        return false;
    }
    // Send data over the stream socket
    ssize_t bytesSent = send(socketFd_, line.c_str(), line.size(), 0);
    if (bytesSent < 0) {
        perror("client write failed");
        this->disconnectFromServer();
        return false;
    }
    return bytesSent==static_cast<ssize_t>(line.size());
}

void IpcClient::disconnectFromServer()
{
    if (socketFd_ >= 0)
    {
        close(socketFd_);
        socketFd_ = -1;
    }
}

bool IpcClient::reconnectToServer()
{
    disconnectFromServer();
    return connectToServer();
}
