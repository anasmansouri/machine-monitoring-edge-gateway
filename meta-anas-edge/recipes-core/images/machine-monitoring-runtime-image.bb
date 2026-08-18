SUMMARY = "Runtime image for the machine monitoring gateway"
DESCRIPTION = "Yocto runtime image containing the edge gateway, ROS2 bridge, and normal customer software."
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

IMAGE_INSTALL += " \
    packagegroup-core-boot \
    edge-gateway \
    ros2-stm32-bridge \
    ros-core \
    static-ethernet \
    iproute2 \
    util-linux \
    bash \
    kbd \
    socat \
"
