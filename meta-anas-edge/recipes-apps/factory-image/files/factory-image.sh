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
DATA_PARTLABEL="data"
DATA_DEVICE="/dev/disk/by-partlabel/${DATA_PARTLABEL}"

RUNTIME_SLOT_A_PARTLABEL="runtimeA"
RUNTIME_SLOT_A_DEVICE="/dev/disk/by-partlabel/${RUNTIME_SLOT_A_PARTLABEL}"
RUNTIME_SLOT_A_REAL_DEVICE=""

FLASHING_ENABLED="0"

BOOT_SWITCH_ENABLED="0"
BOOT_CMDLINE_FILE="/boot/cmdline.txt"
BOOT_CMDLINE_BACKUP="/boot/cmdline.factory.backup"
RUNTIME_SLOT_A_PARTUUID=""

UBOOT_ENV_TOOL="/usr/bin/fw_setenv"
UBOOT_PRINT_TOOL="/usr/bin/fw_printenv"
TARGET_BOOT_SLOT="runtimeA"

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

if [ ! -b "${DATA_DEVICE}" ]; then
    log_msg "FAIL: data partition not found by GPT PARTLABEL=${DATA_PARTLABEL}"
    exit 1
fi

if mountpoint -q "${DATA_MOUNT_POINT}"; then
    log_msg "INFO: ${DATA_MOUNT_POINT} is already mounted"
else
    if mount "${DATA_DEVICE}" "${DATA_MOUNT_POINT}"; then
        log_msg "PASS: permanent data partition mounted at ${DATA_MOUNT_POINT}"
    else
        log_msg "FAIL: could not mount permanent data partition ${DATA_DEVICE}"
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
    exit 1
fi

log_msg "Step 3: Checking STM32 UART device"

if [ -e /dev/ttyAMA2 ]; then
    log_msg "PASS: UART device /dev/ttyAMA2 exists"
else
    log_msg "FAIL: UART device /dev/ttyAMA2 not found"
    exit 1
fi

log_msg "Step 3.1: Checking STM32 hardware"

if /usr/bin/factory-stm32-check >> "${REPORT_FILE}" 2>&1; then
    log_msg "PASS: STM32 board passed factory hardware test"
else
    log_msg "FAIL: STM32 board failed factory hardware test"
    exit 1
fi

log_msg "Step 4: Preparing customer software mount point"

mkdir -p "${MOUNT_POINT}"
log_msg "PASS: ${MOUNT_POINT} is ready"

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

log_msg "Step 8: Preparing runtime partition flashing target"

if [ ! -b "${RUNTIME_SLOT_A_DEVICE}" ]; then
    log_msg "FAIL: runtimeA partition not found by GPT PARTLABEL=${RUNTIME_SLOT_A_PARTLABEL}"
    exit 1
fi

RUNTIME_SLOT_A_REAL_DEVICE="$(readlink -f "${RUNTIME_SLOT_A_DEVICE}")"

log_msg "Runtime slot A PARTLABEL: ${RUNTIME_SLOT_A_PARTLABEL}"
log_msg "Runtime slot A device: ${RUNTIME_SLOT_A_REAL_DEVICE}"

CURRENT_ROOT_DEVICE="$(findmnt -rn -o SOURCE /)"
CURRENT_ROOT_REAL_DEVICE="$(readlink -f "${CURRENT_ROOT_DEVICE}")"

log_msg "Current root device: ${CURRENT_ROOT_REAL_DEVICE}"

if [ "${RUNTIME_SLOT_A_REAL_DEVICE}" = "${CURRENT_ROOT_REAL_DEVICE}" ]; then
    log_msg "FAIL: runtime target is the currently booted root filesystem, refusing to flash"
    exit 1
fi

RUNTIME_IMAGE_FILE_SIZE="$(stat -c%s "${RUNTIME_IMAGE_FILE}")"
RUNTIME_SLOT_A_SIZE="$(blockdev --getsize64 "${RUNTIME_SLOT_A_REAL_DEVICE}")"

log_msg "Runtime image size: ${RUNTIME_IMAGE_FILE_SIZE} bytes"
log_msg "Runtime slot A partition size: ${RUNTIME_SLOT_A_SIZE} bytes"

if [ "${RUNTIME_IMAGE_FILE_SIZE}" -gt "${RUNTIME_SLOT_A_SIZE}" ]; then
    log_msg "FAIL: runtime image is larger than runtimeA partition, cannot flash"
    exit 1
fi

log_msg "PASS: runtime image fits into runtimeA partition"

if findmnt -rn -S "${RUNTIME_SLOT_A_REAL_DEVICE}" >/dev/null 2>&1; then
    log_msg "FAIL: runtimeA partition ${RUNTIME_SLOT_A_REAL_DEVICE} is mounted, cannot flash"
    exit 1
fi

log_msg "PASS: runtimeA partition is not mounted"

if [ "${FLASHING_ENABLED}" != "1" ]; then
    log_msg "DRY-RUN: flashing is disabled"
    log_msg "DRY-RUN: would flash ${RUNTIME_IMAGE_FILE} to ${RUNTIME_SLOT_A_REAL_DEVICE}"
else
    log_msg "Flashing runtime image to ${RUNTIME_SLOT_A_REAL_DEVICE}"

    if dd if="${RUNTIME_IMAGE_FILE}" of="${RUNTIME_SLOT_A_REAL_DEVICE}" bs=4M status=progress conv=fsync; then
        sync
        log_msg "PASS: runtime image flashed successfully"
    else
        log_msg "FAIL: problem while flashing runtime image"
        exit 1
    fi
fi

log_msg "Step 9: Preparing U-Boot boot target switch to ${TARGET_BOOT_SLOT}"

if [ ! -x "${UBOOT_ENV_TOOL}" ]; then
    log_msg "FAIL: fw_setenv not found or not executable: ${UBOOT_ENV_TOOL}"
    exit 1
fi

if [ ! -x "${UBOOT_PRINT_TOOL}" ]; then
    log_msg "FAIL: fw_printenv not found or not executable: ${UBOOT_PRINT_TOOL}"
    exit 1
fi

if [ ! -f "/etc/fw_env.config" ]; then
    log_msg "FAIL: /etc/fw_env.config not found"
    exit 1
fi

log_msg "Current U-Boot boot_slot value:"
if "${UBOOT_PRINT_TOOL}" boot_slot >> "${REPORT_FILE}" 2>&1; then
    "${UBOOT_PRINT_TOOL}" boot_slot | while read -r line; do
        log_msg "${line}"
    done
else
    log_msg "boot_slot is currently not set"
fi

if [ "${BOOT_SWITCH_ENABLED}" != "1" ]; then
    log_msg "DRY-RUN: boot switch is disabled"
    log_msg "DRY-RUN: would run: fw_setenv boot_slot ${TARGET_BOOT_SLOT}"
else
    log_msg "Switching U-Boot boot target to ${TARGET_BOOT_SLOT}"

    if ! "${UBOOT_ENV_TOOL}" boot_slot "${TARGET_BOOT_SLOT}"; then
        log_msg "FAIL: could not set U-Boot boot_slot=${TARGET_BOOT_SLOT}"
        exit 1
    fi

    sync

    log_msg "PASS: U-Boot boot target switched to ${TARGET_BOOT_SLOT}"
    log_msg "Updated U-Boot boot_slot value:"
    "${UBOOT_PRINT_TOOL}" boot_slot | while read -r line; do
        log_msg "${line}"
    done
fi

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
