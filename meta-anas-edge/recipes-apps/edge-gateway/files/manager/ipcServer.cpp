#include "manager/ipcServer.hpp"
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <cstring>
#include <iostream>

namespace cc::manager {

IpcServer::IpcServer(std::string socketPath ,MessageCallback callback)
    : socketPath_(std::move(socketPath)),onMessageReceived_(std::move(callback))
{
}

IpcServer::~IpcServer()
{
    stop();
}

bool IpcServer::start()
{
    unlink(socketPath_.c_str());

    serverFd_ = socket(AF_UNIX, SOCK_STREAM, 0);

    if (serverFd_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    std::strncpy(
        addr.sun_path,
        socketPath_.c_str(),
        sizeof(addr.sun_path) - 1);

    if (bind(
            serverFd_,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) < 0) {
        perror("bind");
        return false;
    }

    if (listen(serverFd_, 5) < 0) {
        perror("listen");
        return false;
    }

    running_ = true;

    acceptThread_ =
        std::thread(&IpcServer::acceptLoop, this);

    return true;
}

void IpcServer::stop()
{
    running_ = false;

    if (serverFd_ >= 0) {
        close(serverFd_);
        serverFd_ = -1;
    }
    //Close all connected clients to wake up and kill their threads
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (int clientFd : clients_) {
            close(clientFd); 
        }
        clients_.clear(); // Empty the vector
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    unlink(socketPath_.c_str());
}

void IpcServer::acceptLoop()
{
    while (running_) {

        int clientFd = accept(serverFd_, nullptr, nullptr);

        if (clientFd < 0) {
            continue;
        }

        {
        std::lock_guard lock(clientsMutex_);

        clients_.push_back(clientFd);
        }

        std::cout
            << "IPC client connected"
            << std::endl;
        std::thread(&IpcServer::readLoop, this, clientFd).detach();
    }
}
void IpcServer::readLoop(int clientFd){

    while(this->running_){

        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));

        ssize_t bytesRead =
            read(clientFd, buffer, sizeof(buffer) - 1);

        if (bytesRead > 0) {
            if(this->onMessageReceived_){
                this->onMessageReceived_(clientFd,std::string(buffer));
            }
            std::cout<< "Client " << clientFd <<" "<<std::string(buffer)<<std::endl;
        }else if(bytesRead==0){
            std::cout << "Client " << clientFd << " disconnected cleanly." << std::endl;
            break;
        }else{
            perror("read error");
            break;
        }

        std::cout<<""<<std::endl;

        }
    this->closeClient(clientFd);
}

void IpcServer::closeClient(int clientFd)
{
    std::lock_guard lock(clientsMutex_); 
    close(clientFd);
    for (auto it = clients_.begin(); it != clients_.end(); ++it) {
        if (*it == clientFd) {
            clients_.erase(it);
            break;
        }
    }
}

bool IpcServer::broadcastLine(const std::string& line)
{
    std::lock_guard<std::mutex> lock(clientsMutex_);

    for (size_t i = 0; i < clients_.size(); ) {

        int clientFd = clients_[i];

        ssize_t ret = send(
            clientFd,
            line.c_str(),
            line.size(),
            0);

        if (ret < 0) {
            close(clientFd);
            clients_.erase(clients_.begin() + i);
        } else {
            i++;
        }
    }

    return true;
}

bool  IpcServer::sendMsg(int clientFd,const std::string& line){
        ssize_t ret = send(
            clientFd,
            line.c_str(),
            line.size(),
            0);

        if (ret < 0) {
            return false;
        }
        return true;
}

}
