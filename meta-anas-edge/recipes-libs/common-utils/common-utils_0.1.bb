SUMMARY = "common utils"
DESCRIPTION = "common utils"
LICENSE = "CLOSED"

SRC_URI = "file://CMakeLists.txt \
           file://utils/Result.cpp \
           file://utils/Result.hpp \
           file://cmake/common_utilsConfig.cmake.in"

S = "${WORKDIR}"

inherit cmake

FILES:${PN} += "${libdir}/libcommon_utils.so.*"
FILES:${PN}-dev += " \
    ${includedir}/common_utils \
    ${libdir}/libcommon_utils.so \
    ${libdir}/cmake/common_utils \
"
