#include "manager/protocolParser.hpp"
#include <cstdio> // Required for sscanf

namespace cc::manager {

    cc::utils::Result<bool> ProtocolParser::isAck(std::string line){
        if(line.starts_with("ACK:")){
            return  cc::utils::Result<bool>::ok(true);
        }
        return  cc::utils::Result<bool>::ok(false);
    }

    cc::utils::Result<bool> ProtocolParser::isAckandPing(std::string line){
     if(line.starts_with("ACK:PING")){
            return  cc::utils::Result<bool>::ok(true);
        }
        return  cc::utils::Result<bool>::ok(false);
    }
    cc::utils::Result<bool> ProtocolParser::isNAck(std::string line){
        if(line.starts_with("NACK:")){
            return  cc::utils::Result<bool>::ok(true);
        }
        return  cc::utils::Result<bool>::ok(false);
    }

    cc::utils::Result<bool> ProtocolParser::isStatus(std::string line){
        if(line.starts_with("STATUS:")){
            return  cc::utils::Result<bool>::ok(true);
        }
        return  cc::utils::Result<bool>::ok(false);
    }

    cc::utils::Result<Telemetry> ProtocolParser::parseStatus(std::string line){
        Telemetry data{};
        int emergency = 0;
        // std::string msg_format ="STATUS:TEMP=%d;HUM=%d;LOAD=%d;STATE=%29[^;];FAULT=%29[^;];OPERATING_MODE=%29[^;];DHT_STATUS=%29[^;];LOAD_STATUS=%29[^;]";
        const char* msg_format =
        "STATUS:TEMP=%d;HUM=%d;LOAD=%d;"
        "VIB_X=%ld;VIB_Y=%ld;VIB_Z=%ld;"
        "fanRPM=%lu;emergency_button=%d;"
        "STATE=%29[^;];FAULT=%29[^;];OPERATING_MODE=%29[^;];"
        "DHT_STATUS=%29[^;];LOAD_STATUS=%29[^;\r\n]";
        //int matched = std::sscanf(line.c_str(), msg_format, &(data.temperature), &(data.humidity),&(data.load),data.machine_state,data.fault,data.operating_mode,data.dht_status,data.load_status);

        int matched = std::sscanf(line.c_str(),
                              msg_format,
                              &data.temperature,
                              &data.humidity,
                              &data.load,
                              &data.vibrationX_mg,
                              &data.vibrationY_mg,
                              &data.vibrationZ_mg,
                              &data.fan_rpm,
                              &emergency,
                              data.machine_state,
                              data.fault,
                              data.operating_mode,
                              data.dht_status,
                              data.load_status);
        data.emergency_button = (emergency != 0);
        if(matched==13){
            return cc::utils::Result<Telemetry>::ok(data);
        }else{
            return cc::utils::Result<Telemetry>::fail(cc::utils::ErrorCode::ParseError,"can't parse data");

        }
    }
}
