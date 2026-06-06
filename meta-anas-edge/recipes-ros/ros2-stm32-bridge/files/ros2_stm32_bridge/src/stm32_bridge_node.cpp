#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "ros2_stm32_bridge/ipc_client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "machine_interfaces/srv/set_load_threshold.hpp"
using namespace std::chrono_literals;

class Stm32BridgeNode : public rclcpp::Node
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
        publisher_ = this->create_publisher<std_msgs::msg::String>(
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
        std_msgs::msg::String message;
        message.data = this->telemetryClient.readLine();
        if (message.data.empty())
        {
            return;
        }

        publisher_->publish(message);
        // this->ipcCommandClient.writeLine("START_MACHINE");

        RCLCPP_INFO(
            this->get_logger(),
            "Published: '%s'",
            message.data.c_str());
    }

    std::string sendCommandToGateway(const std::string &cmd)
    {
        if (!this->commandClient.writeLine(cmd))
        {
            return "";
        }

        return this->commandClient.readLine();
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

    IpcClient telemetryClient;
    IpcClient commandClient;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    // services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr start_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_fault_service_;
    rclcpp::Service<machine_interfaces::srv::SetLoadThreshold>::SharedPtr set_load_threshold_service_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<Stm32BridgeNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
