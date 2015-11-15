# rpi_b
Attempt to port of Minix 3.3.0 to RaspberryPi B+ (BCM2835)

To build it just run ./releasetools/arm_sdimage.sh command from main directory

To load binaries to SD card just use regular command, e.g.:
dd if=minix_arm_sd.img of=/dev/mmcblk0 bs=1M oflag=direct

The Raspberry Pi B+ version is build as evbearmv6hf-el architecture
U-Boot loader allows to update FW through ethernet interface

Output messages are printing out through serial interface of RaspberryPi B+ board
