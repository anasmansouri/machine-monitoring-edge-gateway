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

    while (true)
    {
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
    while (true)
    {
        cmd = "GET_STATUS";
        cc::utils::Result<std::string> response;
        {
            std::lock_guard<std::mutex> lock(uartMutex);

            if (!uartManager.writeLine(cmd))
            {
                log << "writing fail " << std::endl
                    << std::flush;
                break;
            }
            log << "PI Sent : " << cmd << std::endl
                << std::flush;
            response = uartManager.readLine();
        }
        if (!response)
        {
            log << "GET_STATUS read failed, going back to handshake" << std::endl
                << std::flush;
            break;
        }
        if (response)
        {
            log << "STM32 REPLIES : " << response.unwrap() << std::endl
                << std::flush;
            if (protocolParser.isAck(response.unwrap()).unwrap())
            {
                log << "ack response" << std::endl
                    << std::flush;
            }
            else if (protocolParser.isNAck(response.unwrap()).unwrap())
            {
                log << "Nack response" << std::endl
                    << std::flush;
            }
            else if (protocolParser.isStatus(response.unwrap()).unwrap())
            {
                cc::utils::Result<cc::manager::Telemetry> data = protocolParser.parseStatus(response.unwrap());
                if (data)
                {
                    std::string json =
                        "{\"type\":\"machine_snapshot\","
                        "\"temperature\":" +
                        std::to_string(data.unwrap().temperature) +
                        ",\"humidity\":" + std::to_string(data.unwrap().humidity) +
                        ",\"load\":" + std::to_string(data.unwrap().load) +
                        ",\"fan_rpm\":" + std::to_string(data.unwrap().fan_rpm) +
                        ",\"vibration_x_mg\":" + std::to_string(data.unwrap().vibrationX_mg) +
                        ",\"vibration_y_mg\":" + std::to_string(data.unwrap().vibrationY_mg) +
                        ",\"vibration_z_mg\":" + std::to_string(data.unwrap().vibrationZ_mg) +
                        ",\"vibration_level_mg\":" + std::to_string(data.unwrap().vibration_level_mg) +
                        ",\"emergency_button\":" +
                        std::string(data.unwrap().emergency_button ? "true" : "false") +
                        ",\"state\":\"" +
                        std::string(data.unwrap().machine_state) +
                        "\",\"fault\":\"" +
                        std::string(data.unwrap().fault) +
                        "\",\"operating_mode\":\"" +
                        std::string(data.unwrap().operating_mode) +
                        "\",\"dht_status\":\"" +
                        std::string(data.unwrap().dht_status) +
                        "\",\"load_status\":\"" +
                        std::string(data.unwrap().load_status) +
                        "\"}\n";

                    log<<json<<std::endl<<std::flush;
                }
                else
                {
                    log << "can't parse data" << std::endl
                        << std::flush;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    }
}
