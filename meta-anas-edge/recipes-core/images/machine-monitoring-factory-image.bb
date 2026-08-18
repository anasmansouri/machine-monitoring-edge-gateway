SUMMARY = "Factory image for the machine monitoring gateway"
DESCRIPTION = "Minimal Yocto image used for factory checks, NFS runtime image validation, and future installation flow."
LICENSE = "MIT"

inherit core-image

IMAGE_FEATURES += "ssh-server-dropbear debug-tweaks"

IMAGE_INSTALL += " \
    packagegroup-core-boot \
    factory-image \
    factory-stm32-check \
    static-ethernet \
    nfs-utils \
    iproute2 \
    util-linux \
    bash \
    kbd \
    socat \
"
