#!/bin/sh

set -eu

LOG_DIR="/var/log/factory-image"
REPORT_FILE="${LOG_DIR}/factory-image.log"

NFS_SERVER="192.168.50.1"
NFS_EXPORT="/export/customer-software"
MOUNT_POINT="/mnt/customer-software"
MANIFEST_FILE="${MOUNT_POINT}/manifest.json"
RUNTIME_IMAGE_FILE="${MOUNT_POINT}/runtime-rootfs.ext4"

mkdir -p "${LOG_DIR}"

log_msg()
{
    echo "$(date '+%Y-%m-%d %H:%M:%S') | $1" | tee -a "${REPORT_FILE}"
}

log_msg "Factory Image started"
log_msg "Step 1: Checking target environment"

log_msg "Hostname: $(hostname)"
log_msg "Kernel: $(uname -a)"

log_msg "Step 2: Checking Ethernet configuration"

if ip addr show eth0 >/dev/null 2>&1; then
    log_msg "PASS: eth0 exists"
    ip addr show eth0 | tee -a "${REPORT_FILE}"
else
    log_msg "FAIL: eth0 not available"
fi

log_msg "Step 3: Checking STM32 UART device"

if [ -e /dev/ttyAMA0 ]; then
    log_msg "PASS: UART device /dev/ttyAMA0 exists"
else
    log_msg "FAIL: UART device /dev/ttyAMA0 not found"
fi

log_msg "Step 3.1: Checking STM32 hardware"

if  /usr/bin/factory-stm32-check >> "${REPORT_FILE}" 2>&1 ; then
    log_msg "PASS: stm32 board is tested"
else
    log_msg "FAIL: stm32 board is damaged"
    exit 1
fi

log_msg "Step 4: Preparing customer software mount point"

mkdir -p /mnt/customer-software
log_msg "PASS: /mnt/customer-software is ready"

log_msg "Step 5: Mounting customer software NFS export"

if mountpoint -q "${MOUNT_POINT}"; then
    log_msg "INFO: ${MOUNT_POINT} is already mounted"
else
    if mount -t nfs "${NFS_SERVER}:${NFS_EXPORT}" "${MOUNT_POINT}"; then
        log_msg "PASS: NFS export mounted successfully"
    else
        log_msg "FAIL: Could not mount NFS export ${NFS_SERVER}:${NFS_EXPORT}"
        exit 1
    fi
fi

log_msg "Step 6: Checking customer software files"

if [ -f "${MANIFEST_FILE}" ]; then
    log_msg "PASS: manifest.json found"
    cat "${MANIFEST_FILE}" | tee -a "${REPORT_FILE}"
else
    log_msg "FAIL: manifest.json not found"
    exit 1
fi

if [ -f "${RUNTIME_IMAGE_FILE}" ]; then
    log_msg "PASS: runtime-rootfs.ext4 found"
    ls -l "${RUNTIME_IMAGE_FILE}" | tee -a "${REPORT_FILE}"
else
    log_msg "FAIL: runtime-rootfs.ext4 not found"
    exit 1
fi

log_msg "Step 7: Partition flashing not enabled yet"
log_msg "Step 8: U-Boot boot target update not enabled yet"

log_msg "Factory Image finished safely"
exit 0
