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
* End-to-end telemetry from STM32 sensors to ROS2 topics

## System Architecture

```text
+----------------------+
| STM32 Machine I/O    |
| - DHT sensor         |
| - Load sensor        |
| - Fan RPM            |
| - Vibration sensor   |
| - Emergency input    |
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
* Convert STM32 status frames into JSON snapshots
* Broadcast telemetry over Unix socket
* Forward machine control commands to STM32
* Stay alive if STM32 is powered off or disconnected

### 2. ros2-stm32-bridge

ROS2 C++ node.

Responsibilities:

* Read telemetry JSON from the gateway over Unix socket
* Parse machine snapshots
* Publish machine telemetry on ROS2 topic
* Provide ROS2 services for machine control
* Reconnect if the gateway socket is temporarily unavailable

### 3. machine-interfaces

Custom ROS2 interface package.

It contains:

* Custom telemetry message
* Custom service for setting load thresholds

## Telemetry Pipeline

The current end-to-end data flow is:

```text
STM32 sensors
  -> UART STATUS response
  -> Raspberry Pi edge-gateway
  -> Unix socket JSON message
  -> ros2-stm32-bridge
  -> /machine/telemetry
```

The gateway publishes JSON snapshots over IPC, for example:

```json
{"type":"machine_snapshot","temperature":27,"humidity":62,"load":28,"fan_rpm":2100,"vibration_x_mg":374,"vibration_y_mg":-724,"vibration_z_mg":-430,"emergency_button":false,"state":"MACHINE_STATE_IDLE","fault":"FAULT_NONE","operating_mode":"AUTO_MODE","dht_status":"DHT_OK","load_status":"LOAD_OK"}
```

## ROS2 Interfaces

### Topic

```text
/machine/telemetry
```

Publishes machine telemetry received from STM32.

Current fields:

```text
temperature
humidity
load
fan_rpm
vibration_x_mg
vibration_y_mg
vibration_z_mg
emergency_button
state
fault
operating_mode
dht_status
load_status
```

Example ROS2 output:

```yaml
temperature: 27
humidity: 62
load: 28
fan_rpm: 2100
vibration_x_mg: 374
vibration_y_mg: -724
vibration_z_mg: -430
emergency_button: false
state: MACHINE_STATE_IDLE
fault: FAULT_NONE
operating_mode: AUTO_MODE
dht_status: DHT_OK
load_status: LOAD_OK
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
STATUS:TEMP=27;HUM=62;LOAD=28;VIB_X=374;VIB_Y=-724;VIB_Z=-430;fanRPM=2100;emergency_button=0;STATE=MACHINE_STATE_IDLE;FAULT=FAULT_NONE;OPERATING_MODE=AUTO_MODE;DHT_STATUS=DHT_OK;LOAD_STATUS=LOAD_OK
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

The edge gateway also writes an application log:

```bash
tail -f /var/log/edge-gateway.log
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

Build only the gateway or ROS2 packages during development:

```bash
bitbake edge-gateway
bitbake machine-interfaces
bitbake ros2-stm32-bridge
```

Image output:

```text
tmp/deploy/images/raspberrypi5/
```

The latest image symlink is usually:

```text
tmp/deploy/images/raspberrypi5/core-image-base-raspberrypi5.rootfs.wic.bz2
```

## Flash Image

Check the SD card device first:

```bash
lsblk
```

Flash the image:

```bash
sudo bmaptool copy tmp/deploy/images/raspberrypi5/core-image-base-raspberrypi5.rootfs.wic.bz2 /dev/sdX
sync
```

Replace `/dev/sdX` with the correct SD card device, not a partition such as `/dev/sdX1`.

## Runtime Test

After booting the Raspberry Pi:

```bash
systemctl status edge-gateway
systemctl status ros2-stm32-bridge
```

Check that the gateway receives STM32 data:

```bash
tail -n 50 /var/log/edge-gateway.log
```

Expected log pattern:

```text
PI sent PING
STM32 REPLIES : ACK:PING
PI Sent : GET_STATUS
STM32 REPLIES : STATUS:TEMP=...
```

Source ROS2 on the Yocto target. The image may use BusyBox `sh`, so use `setup.sh` instead of `setup.bash`:

```sh
unset AMENT_SHELL
unset AMENT_CURRENT_PREFIX
. /opt/ros/jazzy/setup.sh
export ROS_DOMAIN_ID=7
export ROS_LOCALHOST_ONLY=0
```

Check telemetry:

```bash
ros2 topic list
ros2 interface show machine_interfaces/msg/MachineTelemetry
ros2 topic echo /machine/telemetry
```

Call services:

```bash
ros2 service call /machine/start_machine std_srvs/srv/Trigger "{}"

ros2 service call /machine/stop_machine std_srvs/srv/Trigger "{}"

ros2 service call /machine/reset_fault std_srvs/srv/Trigger "{}"

ros2 service call /machine/set_load_threshold machine_interfaces/srv/SetLoadThreshold "{warning: 60, fault: 85}"
```

## ROS2 Host Communication Test

The Raspberry Pi uses a static Ethernet address:

* Host PC: `192.168.50.1`
* Raspberry Pi: `192.168.50.2`

A ROS2 Jazzy Docker container can communicate with the Raspberry Pi using host networking:

```bash
docker run -it --rm \
  --net=host \
  -e ROS_DOMAIN_ID=7 \
  -e ROS_LOCALHOST_ONLY=0 \
  -v ~/personal_projects/yocto_project/machine-monitoring-edge-gateway:/gateway \
  ros:jazzy-ros-base \
  bash
```

Inside the container:

```bash
source /opt/ros/jazzy/setup.bash
ros2 topic list
ros2 topic echo /machine/telemetry
```

The machine state is controlled by STM32 firmware. `IDLE` is the safe waiting state, `RUNNING` is normal operation, `WARNING` means the machine can still run but a threshold is close, and `FAULT` is a latched unsafe state. Emergency stop and hard faults have the highest priority and force the machine into `FAULT`.

stateDiagram-v2
    [*] --> IDLE: Boot / init OK

    IDLE --> RUNNING: START_MACHINE\nall conditions safe
    IDLE --> FAULT: START_MACHINE\nemergency or sensor fault active

    RUNNING --> IDLE: STOP_MACHINE
    RUNNING --> WARNING: load/temp above\nwarning threshold
    RUNNING --> FAULT: emergency stop\nsensor error\nload/temp fault

    WARNING --> RUNNING: values return\nbelow warning threshold
    WARNING --> IDLE: STOP_MACHINE
    WARNING --> FAULT: emergency stop\nsensor error\nfault threshold reached

    FAULT --> IDLE: RESET_FAULT\nall conditions safe
    FAULT --> FAULT: RESET_FAULT\ncondition still active

    note right of FAULT
      FAULT is latched.
      It does not clear automatically.
      RESET_FAULT is required after
      the system becomes safe again.
    end note

## Hardware

* Raspberry Pi 5
* STM32 Nucleo board
* UART connection between Raspberry Pi and STM32
* DHT11 / KY-015 temperature and humidity sensor
* Load sensor / potentiometer
* ADXL345 vibration sensor
* 4-pin PWM fan with RPM feedback
* Emergency stop input
* Status LEDs

## UART Settings

```text
Baud rate: 115200
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

## Current Status

Implemented and tested:

* STM32 to Raspberry Pi UART handshake with `PING`
* Periodic `GET_STATUS` polling
* Temperature, humidity, and load telemetry
* ADXL345 vibration telemetry on X/Y/Z axes
* Fan RPM and emergency button fields in the protocol and ROS2 message
* Edge gateway protocol parsing
* IPC JSON broadcast
* ROS2 `/machine/telemetry` publishing
* ROS2 service forwarding for machine commands

Planned next steps:

* Add a single vibration level or vibration alert field
* Add dashboard or HMI visualization
* Add long-term metrics storage with InfluxDB/Grafana
* Add OTA/FOTA update flow

## Skills Demonstrated

* Embedded Linux
* Yocto Project
* BitBake recipes
* systemd service integration
* C++ on Linux
* UART communication
* Protocol parsing
* Unix socket IPC
* JSON telemetry bridge
* ROS2 C++ nodes
* Custom ROS2 messages and services
* STM32 integration
* Sensor telemetry integration
* Robust reconnect and timeout handling
