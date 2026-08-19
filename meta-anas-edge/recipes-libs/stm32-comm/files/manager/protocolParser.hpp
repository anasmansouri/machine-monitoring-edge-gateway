#pragma once
#include <cstdint>
#include <string>
#include "utils/Result.hpp"

namespace cc::manager{
enum class MSG_TYPE:uint8_t{
    ACK,
    NACK,
    STATUS,
    UNKNOWN
};
struct Telemetry{
    int temperature;
    int humidity;
    int load;
    uint32_t fan_rpm = 0;
    int32_t vibrationX_mg = 0;
    int32_t vibrationY_mg = 0;
    int32_t vibrationZ_mg = 0;
    int32_t vibration_level_mg = 0;
    bool emergency_button = false;
    char machine_state[30];
    char fault[30];
    char operating_mode[30];
    char load_status[30];
    char dht_status[30];
};
class ProtocolParser{
    public:
    ProtocolParser()=default;
    ~ProtocolParser()=default;
    static cc::utils::Result<bool> isAck(std::string line);
    static cc::utils::Result<bool> isAckandPing(std::string line);
    static cc::utils::Result<bool> isNAck(std::string line);
    static cc::utils::Result<bool> isStatus(std::string line);
    cc::utils::Result<Telemetry> parseStatus(std::string line);
};
}
