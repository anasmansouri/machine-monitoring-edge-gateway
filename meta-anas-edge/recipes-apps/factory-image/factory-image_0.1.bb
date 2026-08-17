SUMMARY = "Factory image support for machine monitoring gateway"
DESCRIPTION = "Runs factory image checks and prepares the system for runtime software installation."
LICENSE = "CLOSED"

SRC_URI = "file://factory-image.sh \
           file://factory-image.service"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "factory-image.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/factory-image.sh \
        ${D}${bindir}/factory-image.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/factory-image.service \
        ${D}${systemd_system_unitdir}/factory-image.service
}

FILES:${PN} += "${bindir}/factory-image.sh"
FILES:${PN} += "${systemd_system_unitdir}/factory-image.service"
