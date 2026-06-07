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
        std::string msg_format ="STATUS:TEMP=%d;HUM=%d;LOAD=%d;STATE=%29[^;];FAULT=%29[^;];OPERATING_MODE=%29[^;];DHT_STATUS=%29[^;];LOAD_STATUS=%29[^;]";
        int matched = std::sscanf(line.c_str(), msg_format.c_str(), &(data.temperature), &(data.humidity),&(data.load),data.machine_state,data.fault,data.operating_mode,data.dht_status,data.load_status);
        if(matched==8){
            return cc::utils::Result<Telemetry>::ok(data);
        }else{
            return cc::utils::Result<Telemetry>::fail(cc::utils::ErrorCode::ParseError,"can't parse data");

        }
    }
}
