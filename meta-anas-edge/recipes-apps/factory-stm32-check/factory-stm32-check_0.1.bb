SUMMARY = "Simple factory stm32 check tool"
DESCRIPTION = "A basic factory test tool"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2e7f3427ab08fda49476f7eec09fe84c"

SRC_URI = "file://CMakeLists.txt \
           file://main.cpp \
           file://LICENSE"

S = "${WORKDIR}"

inherit cmake 

DEPENDS += "stm32-comm"
RDEPENDS:${PN} += "stm32-comm"
