SUMMARY = "Custom ROS2 interfaces for machine monitoring"
DESCRIPTION = "ROS2 service and message interfaces for the STM32 machine monitoring project."
LICENSE = "CLOSED"

SRC_URI = "file://machine_interfaces"

S = "${WORKDIR}/machine_interfaces"

inherit ros_distro_jazzy
inherit ros_ament_cmake

DEPENDS += " \
    ament-cmake-native \
    ament-cmake-ros \
    ament-cmake-gmock \
    ament-cmake-gtest \
    ament-cmake-pytest \
    python3-numpy-native \
    rosidl-default-generators \
    rosidl-default-runtime \
    rosidl-core-generators \
    rosidl-core-runtime \
    rosidl-adapter \
    rosidl-adapter-native \
    rosidl-generator-c \
    rosidl-generator-c-native \
    rosidl-generator-cpp \
    rosidl-generator-cpp-native \
    rosidl-generator-py \
    rosidl-generator-py-native \
    rosidl-generator-type-description \
    rosidl-generator-type-description-native \
    rosidl-runtime-c \
    rosidl-runtime-cpp \
    rosidl-typesupport-c \
    rosidl-typesupport-c-native \
    rosidl-typesupport-cpp \
    rosidl-typesupport-cpp-native \
    rosidl-typesupport-fastrtps-c \
    rosidl-typesupport-fastrtps-c-native \
    rosidl-typesupport-fastrtps-cpp \
    rosidl-typesupport-fastrtps-cpp-native \
    rosidl-typesupport-introspection-c \
    rosidl-typesupport-introspection-c-native \
    rosidl-typesupport-introspection-cpp \
    rosidl-typesupport-introspection-cpp-native \
    type-description-interfaces \
    fastrtps-cmake-module \
    fastrtps \
    fastcdr \
    service-msgs \
"

RDEPENDS:${PN} += " \
    rosidl-default-runtime \
    rosidl-core-runtime \
    rosidl-runtime-c \
    rosidl-runtime-cpp \
    service-msgs \
    python3-numpy \
    type-description-interfaces \
"
SOLIBS = ".so"
FILES_SOLIBSDEV = ""

FILES:${PN} += " \
    /opt/ros/${ROS_DISTRO}/lib/*.so \
"

INSANE_SKIP:${PN} += "dev-so"
