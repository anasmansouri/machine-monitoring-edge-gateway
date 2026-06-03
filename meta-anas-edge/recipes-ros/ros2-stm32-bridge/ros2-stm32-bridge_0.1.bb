SUMMARY = "Simple ROS2 Jazzy Hello World publisher"
DESCRIPTION = "A minimal ROS2 Jazzy C++ publisher node for testing ROS2 inside a Yocto Raspberry Pi image."
LICENSE = "CLOSED"

SRC_URI = "file://ros2_stm32_bridge"

S = "${WORKDIR}/ros2_stm32_bridge"

inherit ros_distro_jazzy
inherit ros_ament_cmake

DEPENDS += " \
    ament-cmake-native \
    rclcpp \
    std-msgs \
"

RDEPENDS:${PN} += " \
    rclcpp \
    std-msgs \
"

EXTRA_OECMAKE += " \
    -DBUILD_TESTING=OFF \
"
