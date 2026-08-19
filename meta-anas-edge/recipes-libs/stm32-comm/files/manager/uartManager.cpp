#include "manager/uartManager.hpp"
#include <cstring>
#include <format>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
namespace cc::manager
{

    // Constructor
    UartManager::UartManager(std::string uart_device) : uart_device{uart_device}
    {
        this->fd=-1;
    }
    
    //Destructor
    UartManager::~UartManager(){
        if(this->fd>=0){
            close(this->fd);
        }
    }

    // getters and setters
    cc::utils::Result<std::string> UartManager::getUartDevice()
    {
        return cc::utils::Result<std::string>::ok(this->uart_device);
    }
    cc::utils::Result<void> UartManager::setUartDevice(std::string uart_device)
    {
        this->uart_device = uart_device;
        return cc::utils::Result<void>::ok();
    }

    // Open Uart Device
    cc::utils::Result<int> UartManager::openUartDevice()
    {
        this->fd = open(this->uart_device.c_str(), O_RDWR | O_NOCTTY);
        if (this->fd < 0)
        {
            // std::string msg_error = std::format("Failed to open UART: {}",uart_device);
            return cc::utils::Result<int>::fail(cc::utils::ErrorCode::DeviceOpenFailed, std::format("Failed to open UART: {}",uart_device));
        }
        return cc::utils::Result<int>::ok(this->fd);
    }

    // configure uart device
    cc::utils::Result<void> UartManager::configureUartDevice()
    {
        if(this->fd<0){
            return cc::utils::Result<void>::fail(cc::utils::ErrorCode::DeviceNotOpen, "File is not open");
        }
        termios tty{};

        if (tcgetattr(this->fd, &tty) != 0)
        {
            close(fd);
            fd=-1;
            return cc::utils::Result<void>::fail(cc::utils::ErrorCode::UartConfigFailed, "Configuration failed");
        }

        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        tty.c_cflag |= (CLOCAL | CREAD);

        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;

        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        tty.c_lflag = 0;
        tty.c_iflag = 0;
        tty.c_oflag = 0;

        tty.c_cc[VMIN] = 1;
        tty.c_cc[VTIME] = 0;

        if(tcsetattr(this->fd, TCSANOW, &tty)!=0){
            return cc::utils::Result<void>::fail(cc::utils::ErrorCode::UartConfigFailed, "Configuration failed");
        }
        tcflush(fd, TCIOFLUSH);
        return cc::utils::Result<void>::ok();
    }

    // read and write from uart device
    cc::utils::Result<void> UartManager::writeLine(std::string msg)
    {
        std::string line = std::format("{}\n",msg);
        int num_bytes_written = write(this->fd, line.c_str(),line.length());
        if (num_bytes_written < 0)
        {
            return cc::utils::Result<void>::fail(cc::utils::ErrorCode::UartWriteFailed, "write failed");
        }
        else
        {
            return cc::utils::Result<void>::ok();
        }
    }
    cc::utils::Result<std::string> UartManager::readLine(){
        char buffer={0};
        std::string msg;
        while (true) {
            struct pollfd pfd;
            pfd.fd = this->fd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            int poll_result = poll(&pfd, 1, 1000); // timeout: 1000 ms
            if(poll_result==0){
                return cc::utils::Result<std::string>::fail(cc::utils::ErrorCode::UartReadFailed,"UART read Timeout");
            }else if(poll_result<0){
                return cc::utils::Result<std::string>::fail(cc::utils::ErrorCode::UartReadFailed,"UART poll Failed");
            }else if(!(pfd.revents & POLLIN)){
                return cc::utils::Result<std::string>::fail(cc::utils::ErrorCode::UartReadFailed,"UART not ready for reading");
            }

            int num_bytes = read(this->fd,&buffer, 1); 
                if(num_bytes<=0){
                     return cc::utils::Result<std::string>::fail(cc::utils::ErrorCode::UartReadFailed,"can't read anything ");
                }else if (buffer=='\n') {
                    break;
                }else if (buffer=='\r') {
                    continue;
                }
            msg.push_back(buffer);
        }
        return cc::utils::Result<std::string>::ok(msg);
    }
}
