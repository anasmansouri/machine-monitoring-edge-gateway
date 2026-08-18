#include <iterator>
#include <ostream>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "manager/uartManager.hpp"
#include "manager/protocolParser.hpp"
#include "utils/Result.hpp"
#include <unistd.h>
#include <cstring>
#include <thread>
#include <chrono>

std::mutex uartMutex;
// std::ofstream log("/var/log/factory-stm32-check.log", std::ios::app);
std::ostream &log = std::cout;
cc::manager::UartManager uartManager("/dev/ttyAMA0");
cc::manager::ProtocolParser protocolParser;

std::string command_to_send_back;
int main()
{

    if (!uartManager.openUartDevice())
    {
        return -1;
    }
    if (!uartManager.configureUartDevice())
    {
        return -1;
    }
    sleep(3);

    // handschake
    while (true)
    {
        if (!uartManager.writeLine("PING"))
        {
            log << "PI can' write" << std::endl
                << std::flush;
            continue;
        }
        log << "PI sent PING" << std::endl
            << std::flush;
        cc::utils::Result<std::string> response = uartManager.readLine();
        if (!response)
        {
            log << "PI can' read" << std::endl
                << std::flush;
            continue;
        }
        if (protocolParser.isAckandPing(response.unwrap()))
        {
            log << "PING received : " << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::string cmd;
    cmd = "GET_STATUS";
    cc::utils::Result<std::string> response;
    {
        std::lock_guard<std::mutex> lock(uartMutex);

        if (!uartManager.writeLine(cmd))
        {
            log << "writing fail " << std::endl
                << std::flush;
            return -1;
        }
        log << "PI Sent : " << cmd << std::endl
            << std::flush;
        response = uartManager.readLine();
    }
    if (!response)
    {
        log << "GET_STATUS read failed, going back to handshake" << std::endl
            << std::flush;
        return -1;
    }
    if (response)
    {
        log << "STM32 REPLIES : " << response.unwrap() << std::endl
            << std::flush;

        if (protocolParser.isStatus(response.unwrap()).unwrap())
        {
            cc::utils::Result<cc::manager::Telemetry> data = protocolParser.parseStatus(response.unwrap());
            if (data)
            {
                if ((std::string(data.unwrap().dht_status) == "DHT_OK") &&
                    (std::string(data.unwrap().load_status) == "LOAD_OK") &&
                    (std::string(data.unwrap().operating_mode) == "AUTO_MODE") &&
                    (std::string(data.unwrap().fault) == "FAULT_NONE"))
                {
                    log << "Test passed" << std::endl
                        << std::flush;
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                log << "can't parse data" << std::endl
                    << std::flush;
                return -1;
            }
        }
        else
        {
            log << "can't parse data" << std::endl
                << std::flush;
            return -1;
        }
    }
}
