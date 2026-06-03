#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "ros2_stm32_bridge/ipc_client.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class Stm32BridgeNode : public rclcpp::Node
{
public:
    Stm32BridgeNode()
        : Node("stm32_bridge_node"),
        ipcClient("/tmp/stm32-gateway.sock")
    {
        if (!this->ipcClient.connectToServer())
        {
            RCLCPP_ERROR(this->get_logger(), "Failed to connect to IPC server");
        }
        else
        {
            RCLCPP_INFO(this->get_logger(), "Connected to IPC server");
        }
        // connect to server
        publisher_ = this->create_publisher<std_msgs::msg::String>(
            "/machine/telemetry",
            10);

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
        if(message.data.empty()){
            return;
        }

        publisher_->publish(message);

        RCLCPP_INFO(
            this->get_logger(),
            "Published: '%s'",
            message.data.c_str());
    }

    IpcClient ipcClient;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<Stm32BridgeNode>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
