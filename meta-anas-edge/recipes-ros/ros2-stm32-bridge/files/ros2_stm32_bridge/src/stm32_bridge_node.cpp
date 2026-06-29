#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "ros2_stm32_bridge/ipc_client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "machine_interfaces/srv/set_load_threshold.hpp"
#include "machine_interfaces/srv/set_vibration_threshold.hpp"
#include "machine_interfaces/msg/machine_telemetry.hpp"
using namespace std::chrono_literals;

bool parseTelemetryJson(const std::string &json,
                        machine_interfaces::msg::MachineTelemetry &msg)
{
    char state[30] = {0};
    char fault[30] = {0};
    char operatingMode[30] = {0};
    char dhtStatus[30] = {0};
    char loadStatus[30] = {0};
    char emergencyButton[6] = {0}; // "true" or "false"

    int temperature = 0;
    int humidity = 0;
    int load = 0;

    unsigned int fanRpm = 0;
    int vibrationX = 0;
    int vibrationY = 0;
    int vibrationZ = 0;
    int vibrationLEVEL = 0;

    int matched = std::sscanf(
        json.c_str(),
        "{\"type\":\"machine_snapshot\","
        "\"temperature\":%d,"
        "\"humidity\":%d,"
        "\"load\":%d,"
        "\"fan_rpm\":%u,"
        "\"vibration_x_mg\":%d,"
        "\"vibration_y_mg\":%d,"
        "\"vibration_z_mg\":%d,"
        "\"vibration_level_mg\":%d,"
        "\"emergency_button\":%5[^,],"
        "\"state\":\"%29[^\"]\","
        "\"fault\":\"%29[^\"]\","
        "\"operating_mode\":\"%29[^\"]\","
        "\"dht_status\":\"%29[^\"]\","
        "\"load_status\":\"%29[^\"]\"}",
        &temperature,
        &humidity,
        &load,
        &fanRpm,
        &vibrationX,
        &vibrationY,
        &vibrationZ,
        &vibrationLEVEL,
        emergencyButton,
        state,
        fault,
        operatingMode,
        dhtStatus,
        loadStatus);

    if (matched != 14)
    {
        return false;
    }

    msg.temperature = temperature;
    msg.humidity = humidity;
    msg.load = load;

    msg.fan_rpm = fanRpm;
    msg.vibration_x_mg = vibrationX;
    msg.vibration_y_mg = vibrationY;
    msg.vibration_z_mg = vibrationZ;
    msg.vibration_level_mg = vibrationLEVEL;
    msg.emergency_button = (std::string(emergencyButton) == "true");

    msg.state = state;
    msg.fault = fault;
    msg.operating_mode = operatingMode;
    msg.dht_status = dhtStatus;
    msg.load_status = loadStatus;

    return true;
}class Stm32BridgeNode : public rclcpp::Node
{
public:
    Stm32BridgeNode()
        : Node("stm32_bridge_node"),
          telemetryClient("/tmp/stm32-gateway.sock"), commandClient("/tmp/stm32-gateway-command.sock")
    {
        if (!this->telemetryClient.connectToServer())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to connect to IPC server");
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "Connected to IPC server");
        }

        if (!this->commandClient.connectToServer())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to connect to IPC Command server");
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "Connected to IPC  Command server");
        }
        // connect to server
        publisher_ = this->create_publisher<machine_interfaces::msg::MachineTelemetry>(
            "/machine/telemetry",
            10);

        start_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/machine/start_machine",
            std::bind(&Stm32BridgeNode::handleStartMachine,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));

        stop_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/machine/stop_machine",
            std::bind(&Stm32BridgeNode::handleStopMachine,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));

        reset_fault_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/machine/reset_fault",
            std::bind(&Stm32BridgeNode::handleResetFault,
                      this,
                      std::placeholders::_1,
                      std::placeholders::_2));
        set_load_threshold_service_ =
            this->create_service<machine_interfaces::srv::SetLoadThreshold>(
                "/machine/set_load_threshold",
                std::bind(&Stm32BridgeNode::handleSetLoadThreshold,
                          this,
                          std::placeholders::_1,
                          std::placeholders::_2));

        set_vibration_threshold_service_ =
            this->create_service<machine_interfaces::srv::SetVibrationThreshold>(
                "/machine/set_vibration_threshold",
                std::bind(&Stm32BridgeNode::handleSetVibrationThreshold,
                          this,
                          std::placeholders::_1,
                          std::placeholders::_2));

        timer_ = this->create_wall_timer(
            1s,
            std::bind(&Stm32BridgeNode::publish_message, this));

        RCLCPP_INFO(this->get_logger(), "Stm32BridgeNode publisher started");
    }
    ~Stm32BridgeNode()
    {
    }

private:
    void publish_message()
    {
        machine_interfaces::msg::MachineTelemetry message;
        std::string received_msg = this->telemetryClient.readLine();

        if (received_msg.empty())
        {
            RCLCPP_WARN(this->get_logger(),
                        "Telemetry IPC disconnected, trying to reconnect...");

            if (!this->telemetryClient.reconnectToServer())
            {
                RCLCPP_WARN(this->get_logger(),
                            "Telemetry IPC reconnect failed");
                return;
            }

            RCLCPP_INFO(this->get_logger(),
                        "Telemetry IPC reconnected");
            return;
        }

        if (!parseTelemetryJson(received_msg, message))
        {
            RCLCPP_WARN(this->get_logger(),
                        "Failed to parse telemetry JSON: %s",
                        received_msg.c_str());
            return;
        }

        publisher_->publish(message);

        RCLCPP_INFO(this->get_logger(),
                    "Published telemetry: temp=%d hum=%d load=%d state=%s fault=%s mode=%s dht=%s load_status=%s",
                    message.temperature,
                    message.humidity,
                    message.load,
                    message.state.c_str(),
                    message.fault.c_str(),
                    message.operating_mode.c_str(),
                    message.dht_status.c_str(),
                    message.load_status.c_str());
    }

    std::string sendCommandToGateway(const std::string &cmd)
    {
        if (!this->commandClient.writeLine(cmd))
        {
            RCLCPP_WARN(this->get_logger(),
                        "Command IPC write failed, trying to reconnect...");

            if (!this->commandClient.reconnectToServer())
            {
                RCLCPP_WARN(this->get_logger(),
                            "Command IPC reconnect failed");
                return "";
            }

            if (!this->commandClient.writeLine(cmd))
            {
                RCLCPP_WARN(this->get_logger(),
                            "Command IPC write failed after reconnect");
                return "";
            }
        }

        std::string reply = this->commandClient.readLine();

        if (!reply.empty())
        {
            return reply;
        }

        RCLCPP_WARN(this->get_logger(),
                    "Command IPC read failed, trying to reconnect...");

        if (!this->commandClient.reconnectToServer())
        {
            RCLCPP_WARN(this->get_logger(),
                        "Command IPC reconnect failed");
        }

        return "";
    }

    void handleCommandService(const std::string &cmd, std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        std::string reply = this->sendCommandToGateway(cmd);
        if (!reply.empty())
        {
            response->message = reply;
            response->success = reply.rfind("ACK:", 0) == 0;

            RCLCPP_INFO(this->get_logger(),
                        "Command '%s' -> '%s'",
                        cmd.c_str(),
                        reply.c_str());
        }
        else
        {
            response->success = false;
            response->message = "No reposnse from gateway command server";
            return;
        }
    }
    void handleStartMachine(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request;
        handleCommandService("START_MACHINE", response);
    }

    void handleStopMachine(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request;
        handleCommandService("STOP_MACHINE", response);
    }

    void handleResetFault(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request;
        handleCommandService("RESET_FAULT", response);
    }
    void handleSetLoadThreshold(
        const std::shared_ptr<machine_interfaces::srv::SetLoadThreshold::Request> request,
        std::shared_ptr<machine_interfaces::srv::SetLoadThreshold::Response> response)
    {
        std::string command =
            "SET_LOAD_THRESHOLD:WARN=" + std::to_string(request->warning) +
            ";FAULT=" + std::to_string(request->fault);

        std::string reply = sendCommandToGateway(command);

        if (reply.empty())
        {
            response->success = false;
            response->message = "No response from gateway command server";
            return;
        }

        response->message = reply;
        response->success = reply.rfind("ACK:", 0) == 0;

        RCLCPP_INFO(this->get_logger(),
                    "Command '%s' -> '%s'",
                    command.c_str(),
                    reply.c_str());
    }

    void handleSetVibrationThreshold(
        const std::shared_ptr<machine_interfaces::srv::SetVibrationThreshold::Request> request,
        std::shared_ptr<machine_interfaces::srv::SetVibrationThreshold::Response> response)
    {
     //    SET_VIBRATION_THRESHOLD
        std::string command =
            "SET_VIBRATION_THRESHOLD:WARN=" + std::to_string(request->warning) +
            ";FAULT=" + std::to_string(request->fault);

        std::string reply = sendCommandToGateway(command);

        if (reply.empty())
        {
            response->success = false;
            response->message = "No response from gateway command server";
            return;
        }

        response->message = reply;
        response->success = reply.rfind("ACK:", 0) == 0;

        RCLCPP_INFO(this->get_logger(),
                    "Command '%s' -> '%s'",
                    command.c_str(),
                    reply.c_str());
    }

    /*
void handleSetVibrationThreshold(
     const std::shared_ptr<machine_interfaces::srv::SetVibrationThreshold::Request> request,
        std::shared_ptr<machine_interfaces::srv::SetVibrationThreshold::Response> response)
    {
        std::string command =
            "SET_VIBRATION_THRESHOLD:WARN=" + std::to_string(request->warning) +
            ";FAULT=" + std::to_string(request->fault);

        std::string reply = sendCommandToGateway(command);

        if (reply.empty())
        {
            response->success = false;
            response->message = "No response from gateway command server";
            return;
        }

        response->message = reply;
        response->success = reply.rfind("ACK:", 0) == 0;

        RCLCPP_INFO(this->get_logger(),
                    "Command '%s' -> '%s'",
                    command.c_str(),
                    reply.c_str());


    }
    */

    IpcClient telemetryClient;
    IpcClient commandClient;
    rclcpp::Publisher<machine_interfaces::msg::MachineTelemetry>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    // services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr start_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_fault_service_;
    rclcpp::Service<machine_interfaces::srv::SetLoadThreshold>::SharedPtr set_load_threshold_service_;
    rclcpp::Service<machine_interfaces::srv::SetVibrationThreshold>::SharedPtr set_vibration_threshold_service_;
    // rclcpp::Service<machine_interfaces::srv::SetLoadThreshold>::SharedPtr set_vibration_threshold_service_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<Stm32BridgeNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
