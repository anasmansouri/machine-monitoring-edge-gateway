#include "manager/protocolParser.hpp"
#include <cinttypes>
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
       const char* msg_format =
       "STATUS:TEMP=%d;HUM=%d;LOAD=%d;"
       "VIB_X=%" SCNd32 ";VIB_Y=%" SCNd32 ";VIB_Z=%" SCNd32 ";VIB_LEVEL=%" SCNd32 ";"
       "fanRPM=%" SCNu32 ";emergency_button=%d;"
       "STATE=%29[^;];FAULT=%29[^;];OPERATING_MODE=%29[^;];"
       "DHT_STATUS=%29[^;];LOAD_STATUS=%29[^\r\n]";

        int matched = std::sscanf(line.c_str(),
                              msg_format,
                              &data.temperature,
                              &data.humidity,
                              &data.load,
                              &data.vibrationX_mg,
                              &data.vibrationY_mg,
                              &data.vibrationZ_mg,
                              &data.vibration_level_mg,
                              &data.fan_rpm,
                              &emergency,
                              data.machine_state,
                              data.fault,
                              data.operating_mode,
                              data.dht_status,
                              data.load_status);
        data.emergency_button = (emergency != 0);
        if(matched==14){
            return cc::utils::Result<Telemetry>::ok(data);
        }else{
            return cc::utils::Result<Telemetry>::fail(cc::utils::ErrorCode::ParseError,"can't parse data");

        }
    }
}
