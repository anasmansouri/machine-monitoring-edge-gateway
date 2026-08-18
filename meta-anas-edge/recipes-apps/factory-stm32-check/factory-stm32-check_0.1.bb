SUMMARY = "Simple factory stm32 check tool"
DESCRIPTION = "A basic factory test tool"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2e7f3427ab08fda49476f7eec09fe84c"

SRC_URI = "file://CMakeLists.txt \
           file://main.cpp \
           file://manager/uartManager.cpp \
           file://manager/uartManager.hpp \
           file://manager/protocolParser.cpp \
           file://manager/protocolParser.hpp \
           file://utils/Result.cpp \
           file://utils/Result.hpp \
           file://LICENSE"

S = "${WORKDIR}"

inherit cmake 
