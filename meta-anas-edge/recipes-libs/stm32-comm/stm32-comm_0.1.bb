SUMMARY = "Shared STM32 communication library"
DESCRIPTION = "Reusable UART manager, protocol parser, and result utility for STM32 communication."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2e7f3427ab08fda49476f7eec09fe84c"

SRC_URI = "file://CMakeLists.txt \
           file://manager/uartManager.cpp \
           file://manager/uartManager.hpp \
           file://manager/protocolParser.cpp \
           file://manager/protocolParser.hpp \
           file://cmake/stm32_commConfig.cmake.in \
           file://LICENSE"

S = "${WORKDIR}"

inherit cmake


DEPENDS +="common-utils"
REDEPENDS:${PN} +="common-utils"


FILES:${PN} += "${libdir}/libstm32_comm.so.*"
FILES:${PN}-dev += " \
    ${includedir}/stm32-comm \
    ${libdir}/libstm32_comm.so \
    ${libdir}/cmake/stm32_comm \
"
