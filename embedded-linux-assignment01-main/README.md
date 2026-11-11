# Embedded Linux - Assignment 01

## How to Write a Device Driver

Two Linux kernel modules developed and tested on QEMU ARMv7 with Linux 5.15.

## Task 1: Character Device with IOCTL

- Device: /dev/asgn1
- Major number: 241
- Procfs: /proc/asgn1_info
- IOCTL: RESET_BUFFER, GET_STATS, SET_MODE, GET_VERSION
- Invalid IOCTL returns ENOTTY

## Task 2: Misc Counter Device

- Device: /dev/counter
- Major number: 10
- Dynamic minor number
- Procfs: /proc/counter_info
- Read increments the counter
- Write sets a non-negative counter value
- Invalid input is rejected

## Environment

- Ubuntu
- Linux Kernel 5.15
- ARMv7 / QEMU vexpress-a9
- arm-linux-gnueabihf-gcc

## Testing

Task 1: PASS

Task 2: PASS

Both drivers were tested successfully in QEMU.
