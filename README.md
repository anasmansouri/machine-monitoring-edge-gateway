# Machine Monitoring Edge Gateway

Embedded Linux edge gateway running on a Raspberry Pi 5 with a custom Yocto image.

The gateway communicates with an STM32 board over UART, receives machine telemetry, exposes the data through ROS2, and provides ROS2 services to control the machine.

## Project Goal

This project demonstrates a small industrial-style embedded system using:

* Raspberry Pi 5 as an embedded Linux gateway
* Custom Yocto image
* C++ gateway application
* UART communication with STM32
* Unix socket IPC
* ROS2 telemetry publishing
* ROS2 services for machine control
* systemd services for automatic startup
* Robust startup when STM32 is offline

## System Architecture

```text
+----------------------+
| STM32 Machine I/O    |
| - Sensors            |
| - Machine state      |
| - UART protocol      |
+----------+-----------+
           |
           | UART
           |
+----------v-----------+
| Raspberry Pi 5       |
| Yocto Linux          |
|                      |
| edge-gateway         |
| - UART manager       |
| - Protocol parser    |
| - IPC server         |
+----------+-----------+
           |
           | Unix sockets
           |
+----------v-----------+
| ROS2 STM32 Bridge    |
| - /machine/telemetry |
| - ROS2 services      |
+----------------------+
```

## Related Repository

The STM32 firmware used with this gateway is available here:

```text
https://github.com/anasmansouri/stm32-machine-io-node
```

## Main Components

### 1. edge-gateway

C++ application running on the Raspberry Pi.

Responsibilities:

* Open and configure UART
* Perform handshake with STM32 using `PING`
* Poll STM32 using `GET_STATUS`
* Parse machine telemetry
* Broadcast telemetry over Unix socket
* Forward machine control commands to STM32
* Stay alive if STM32 is powered off or disconnected

### 2. ros2-stm32-bridge

ROS2 C++ node.

Responsibilities:

* Read telemetry from the gateway over Unix socket
* Publish machine telemetry on ROS2 topic
* Provide ROS2 services for machine control
* Reconnect if the gateway socket is temporarily unavailable

### 3. machine-interfaces

Custom ROS2 interface package.

It contains:

* Custom telemetry message
* Custom service for setting load thresholds

## ROS2 Interfaces

### Topic

```text
/machine/telemetry
```

Publishes machine telemetry received from STM32.

Example fields:

```text
temperature
humidity
load
state
fault
operating_mode
dht_status
load_status
```

### Services

```text
/machine/start_machine
/machine/stop_machine
/machine/reset_fault
/machine/set_load_threshold
```

## UART Protocol

### Commands sent to STM32

```text
PING
GET_STATUS
START_MACHINE
STOP_MACHINE
RESET_FAULT
SET_LOAD_THRESHOLD:WARN=<value>;FAULT=<value>
```

### Example responses from STM32

```text
ACK:PING
ACK:START_MACHINE
ACK:STOP_MACHINE
ACK:RESET_FAULT
ACK:SET_LOAD_THRESHOLD
NACK:UNKNOWN_CMD
STATUS:TEMP=24;HUM=50;LOAD=35;STATE=MACHINE_STATE_RUNNING;FAULT=FAULT_NONE;OPERATING_MODE=AUTO_MODE;DHT_STATUS=DHT_OK;LOAD_STATUS=LOAD_OK
```

## Robust Startup Behavior

The Raspberry Pi can boot even if the STM32 is not powered on.

Behavior:

* `edge-gateway.service` starts normally.
* The gateway keeps sending `PING`.
* If STM32 is off, UART read timeout prevents blocking forever.
* When STM32 is powered on later, handshake succeeds automatically.
* The gateway switches to normal `GET_STATUS` polling.
* If STM32 disconnects during runtime, the gateway goes back to handshake mode.
* The ROS2 bridge also uses IPC read timeouts to avoid blocking forever.

## systemd Services

The project installs services for automatic startup:

```text
edge-gateway.service
ros2-stm32-bridge.service
```

Check service status:

```bash
systemctl status edge-gateway
systemctl status ros2-stm32-bridge
```

Follow logs:

```bash
journalctl -u edge-gateway -f
journalctl -u ros2-stm32-bridge -f
```

## Yocto Build

Enter the build directory:

```bash
cd build
```

Build the full image:

```bash
bitbake core-image-base
```

Image output:

```text
tmp/deploy/images/raspberrypi5/
```

## Flash Image

Check the SD card device first:

```bash
lsblk
```

Flash the image:

```bash
sudo bmaptool copy tmp/deploy/images/raspberrypi5/*.wic.bz2 /dev/sdX
sync
```

Replace `/dev/sdX` with the correct SD card device.

## Runtime Test

After booting the Raspberry Pi:

```bash
systemctl status edge-gateway
systemctl status ros2-stm32-bridge
```

Source ROS2:

```bash
source /opt/ros/jazzy/setup.bash
```

Check telemetry:

```bash
ros2 topic echo /machine/telemetry
```

Call services:

```bash
ros2 service call /machine/start_machine std_srvs/srv/Trigger "{}"

ros2 service call /machine/stop_machine std_srvs/srv/Trigger "{}"

ros2 service call /machine/reset_fault std_srvs/srv/Trigger "{}"

ros2 service call /machine/set_load_threshold machine_interfaces/srv/SetLoadThreshold "{warning: 60, fault: 85}"
```

## Hardware

* Raspberry Pi 5
* STM32 Nucleo board
* UART connection between Raspberry Pi and STM32
* DHT11 / KY-015 temperature and humidity sensor
* Load sensor / potentiometer
* Status LEDs

## UART Settings

```text
Baud rate: 115200
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

## Skills Demonstrated

* Embedded Linux
* Yocto Project
* BitBake recipes
* systemd service integration
* C++ on Linux
* UART communication
* Unix socket IPC
* ROS2 C++ nodes
* Custom ROS2 messages and services
* STM32 integration
* Robust reconnect and timeout handling

