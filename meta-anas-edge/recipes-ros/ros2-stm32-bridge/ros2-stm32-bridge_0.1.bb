SUMMARY = "Simple ROS2 Jazzy Hello World publisher"
DESCRIPTION = "A minimal ROS2 Jazzy C++ publisher node for testing ROS2 inside a Yocto Raspberry Pi image."
LICENSE = "CLOSED"

SRC_URI = "file://ros2_stm32_bridge \
           file://ros2_stm32_bridge/ros2-stm32-bridge.service"

S = "${WORKDIR}/ros2_stm32_bridge"

inherit ros_distro_jazzy
inherit ros_ament_cmake


DEPENDS += " \
    ament-cmake-native \
    rclcpp \
    std-msgs \
    std-srvs \
    machine-interfaces \
"

RDEPENDS:${PN} += " \
    rclcpp \
    std-msgs \
    std-srvs \
    machine-interfaces \
"

EXTRA_OECMAKE += " \
    -DBUILD_TESTING=OFF \
"

SYSTEMD_SERVICE:${PN} = "ros2-stm32-bridge.service"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/ros2-stm32-bridge.service \
        ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${systemd_system_unitdir}/ros2-stm32-bridge.service"

SYSTEMD_AUTO_ENABLE = "enable"
