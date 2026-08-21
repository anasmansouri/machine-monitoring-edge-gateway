#!/bin/sh

set -eu

LOG_DIR="/var/log/factory-image"
REPORT_FILE="${LOG_DIR}/factory-image.log"

NFS_SERVER="192.168.50.1"
NFS_EXPORT="/export/customer-software"
MOUNT_POINT="/mnt/customer-software"
MANIFEST_FILE="${MOUNT_POINT}/manifest.json"
RUNTIME_IMAGE_FILE="${MOUNT_POINT}/runtime-rootfs.ext3"
EXPECTED_IMAGE_NAME=""
EXPECTED_SHA256=""
ACTUAL_SHA256=""

DATA_MOUNT_POINT="/data"
DATA_REPORT_DIR="${DATA_MOUNT_POINT}/factory-reports"
DATA_PARTITION_LABEL="data"

mkdir -p "${LOG_DIR}"

log_msg()
{
    echo "$(date '+%Y-%m-%d %H:%M:%S') | $1" | tee -a "${REPORT_FILE}"
}

log_msg "Factory Image started"
log_msg "Step 1: Checking target environment"

log_msg "Hostname: $(hostname)"
log_msg "Kernel: $(uname -a)"

log_msg "Step 1.1: Mounting permanent data partition"

mkdir -p "${DATA_MOUNT_POINT}"

if mountpoint -q "${DATA_MOUNT_POINT}"; then
    log_msg "INFO: ${DATA_MOUNT_POINT} is already mounted"
else
    if mount LABEL="${DATA_PARTITION_LABEL}" "${DATA_MOUNT_POINT}"; then
        log_msg "PASS: permanent data partition mounted at ${DATA_MOUNT_POINT}"
    else
        log_msg "FAIL: could not mount permanent data partition LABEL=${DATA_PARTITION_LABEL}"
        exit 1
    fi
fi

mkdir -p "${DATA_REPORT_DIR}"
log_msg "PASS: factory report directory is ready: ${DATA_REPORT_DIR}"

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

if /usr/bin/factory-stm32-check >> "${REPORT_FILE}" 2>&1 ; then
    log_msg "PASS: STM32 board passed factory hardware test"
else
    log_msg "FAIL: STM32 board failed factory hardware test"
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
    log_msg "PASS: runtime-rootfs.ext3 found"
    ls -l "${RUNTIME_IMAGE_FILE}" | tee -a "${REPORT_FILE}"
else
    log_msg "FAIL: runtime-rootfs.ext3 not found"
    exit 1
fi

log_msg "Step 7: Validating runtime image manifest"

EXPECTED_IMAGE_NAME="$(sed -n 's/.*"image"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${MANIFEST_FILE}" | head -n 1)"
EXPECTED_SHA256="$(sed -n 's/.*"sha256"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' "${MANIFEST_FILE}" | head -n 1)"

if [ -z "${EXPECTED_IMAGE_NAME}" ]; then
    log_msg "FAIL: image field missing in manifest.json"
    exit 1
fi

if [ -z "${EXPECTED_SHA256}" ]; then
    log_msg "FAIL: sha256 field missing in manifest.json"
    exit 1
fi

if [ "${EXPECTED_IMAGE_NAME}" != "runtime-rootfs.ext3" ]; then
    log_msg "FAIL: manifest image name is ${EXPECTED_IMAGE_NAME}, expected runtime-rootfs.ext3"
    exit 1
fi

log_msg "Manifest image: ${EXPECTED_IMAGE_NAME}"
log_msg "Manifest expected SHA256: ${EXPECTED_SHA256}"

ACTUAL_SHA256="$(sha256sum "${RUNTIME_IMAGE_FILE}" | awk '{print $1}')"

log_msg "Runtime image actual SHA256: ${ACTUAL_SHA256}"

if [ "${ACTUAL_SHA256}" = "${EXPECTED_SHA256}" ]; then
    log_msg "PASS: runtime image SHA256 checksum is valid"
else
    log_msg "FAIL: runtime image SHA256 checksum mismatch"
    exit 1
fi


log_msg "Step 8: Partition flashing not enabled yet"
log_msg "Step 9: U-Boot boot target update not enabled yet"

log_msg "Step 10: Saving factory report to permanent data partition"

PERSISTENT_REPORT_FILE="${DATA_REPORT_DIR}/factory-image-$(date '+%Y%m%d-%H%M%S').log"

if cp "${REPORT_FILE}" "${PERSISTENT_REPORT_FILE}"; then
    log_msg "PASS: factory report saved to ${PERSISTENT_REPORT_FILE}"
else
    log_msg "FAIL: could not save factory report to permanent data partition"
    exit 1
fi

log_msg "Factory Image finished safely"
exit 0
