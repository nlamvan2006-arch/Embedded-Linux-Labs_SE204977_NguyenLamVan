# Embedded Linux - Assignment 02

## SMS Sensor Monitoring System

This assignment implements an embedded Linux sensor monitoring system using Linux kernel modules, POSIX threads and POSIX message queues.

### Task 1: Sensor Device Driver

- Device: `/dev/sms_sensor`
- Kernel module: `sms_sensor_driver.ko`
- Procfs interface: `/proc/sms_stats`
- IOCTL command: `SMS_SET_INTERVAL`
- Configurable sensor sampling interval
- Simulated temperature and humidity readings

### Task 2: User-space Application

The application consists of four POSIX threads:

1. **Sensor Thread:** Reads sensor data from the kernel device.
2. **Processor Thread:** Calculates moving averages and detects abnormal values.
3. **Alert Thread:** Receives and displays alerts through POSIX message queues.
4. **Logger Thread:** Records sensor data and processing results.

### Alert Conditions

- Temperature exceeds the configured threshold.
- Humidity exceeds the configured threshold.
- Temperature increases by more than 5 degrees.

### Development Environment

- Host OS: Ubuntu
- Target architecture: ARMv7-A
- Linux kernel: 5.15
- Emulator: QEMU vexpress-a9
- Cross compiler: arm-linux-gnueabihf-gcc
- Programming language: C

### Build

Build the kernel module:

```bash
cd driver
make
```

Build the ARM application:

```bash
cd app
arm-linux-gnueabihf-gcc -static -Wall -Wextra -Werror -pthread main.c sensor.c processor.c alert.c logger.c -o sms_app_arm_static
```

### Run

Load the driver:

```bash
insmod sms_sensor_driver.ko
```

Run the application:

```bash
sms_app -i 500 -t 40 -h 80 -d 10 -v
```

### Test Results

Successfully tested on QEMU ARMv7:

- Sensor data acquisition.
- IOCTL interval configuration.
- Moving average processing.
- Temperature and humidity alerts.
- POSIX message queue communication.
- Logging sensor data.
- Application shutdown.
