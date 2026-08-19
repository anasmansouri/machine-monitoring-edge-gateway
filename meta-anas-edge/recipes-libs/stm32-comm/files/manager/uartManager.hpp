#pragma once
#include <string>

#include "utils/Result.hpp"

namespace cc::manager {

class UartManager{
    private:
        std::string uart_device;
        int fd{-1};
    public:
        explicit UartManager(std::string uart_device);
        UartManager()=default;
        // Open device
        cc::utils::Result<int> openUartDevice();
        // getters and setters 
        cc::utils::Result<std::string> getUartDevice();
        cc::utils::Result<void> setUartDevice(std::string  uart_device);
        // configure uart device
        cc::utils::Result<void> configureUartDevice();
        // read and write from uart device
        cc::utils::Result<void> writeLine(std::string msg);
        cc::utils::Result<std::string> readLine();
        ~UartManager();
};
}
