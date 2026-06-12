SUMMARY = "Static Ethernet configuration for direct host-to-Raspberry Pi connection"
LICENSE = "CLOSED"

SRC_URI = "file://20-eth0-static.network"

do_install() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/20-eth0-static.network \
        ${D}${sysconfdir}/systemd/network/20-eth0-static.network
}

FILES:${PN} += " \
    ${sysconfdir}/systemd/network/20-eth0-static.network \
"

RDEPENDS:${PN} += "systemd"
