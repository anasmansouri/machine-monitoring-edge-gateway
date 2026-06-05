#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "ros2_stm32_bridge/ipc_client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

class Stm32BridgeNode : public rclcpp::Node
{
public:
    Stm32BridgeNode()
        : Node("stm32_bridge_node"),
          ipcClient("/tmp/stm32-gateway.sock"), ipcCommandClient("/tmp/stm32-gateway-command.sock")
    {
        if (!this->ipcClient.connectToServer())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to connect to IPC server");
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "Connected to IPC server");
        }

        if (!this->ipcCommandClient.connectToServer())
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
        message.data = this->ipcClient.readLine();
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
    void handleCommandService(const std::string &cmd, std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {

        if (this->ipcCommandClient.writeLine(cmd))
        {
            std::string reply = this->ipcCommandClient.readLine();
            if (reply.empty())
            {
                response->success = false;
                response->message = "No reposnse from gateway command server";
                return;
            }
            response->message = reply;
            if (reply.rfind("ACK:", 0) == 0)
            {
                response->success = true;
            }
            else
            {
                response->success = false;
            }

            RCLCPP_INFO(this->get_logger(),
                        "Command '%s' -> '%s'",
                        cmd.c_str(),
                        reply.c_str());
        }
        else
        {
            response->success = false;
            response->message = "can't send message correctly";
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

    IpcClient ipcClient;
    IpcClient ipcCommandClient;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    // services
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr start_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr stop_service_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_fault_service_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<Stm32BridgeNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
