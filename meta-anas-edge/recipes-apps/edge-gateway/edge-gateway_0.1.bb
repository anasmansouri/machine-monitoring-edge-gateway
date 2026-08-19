SUMMARY = "Simple edge gateway application"
DESCRIPTION = "A basic C++ edge gateway daemon example"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2e7f3427ab08fda49476f7eec09fe84c"

SRC_URI = "file://CMakeLists.txt \
           file://main.cpp \
           file://manager/ipcServer.hpp \
           file://manager/ipcServer.cpp \
           file://LICENSE \
           file://edge-gateway.service"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "edge-gateway.service"

DEPENDS += "stm32-comm"
RDEPENDS:${PN} += "stm32-comm"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/edge-gateway.service \
        ${D}${systemd_system_unitdir}
}

FILES:${PN} += "${systemd_system_unitdir}/edge-gateway.service"

SYSTEMD_AUTO_ENABLE = "enable"
