#!/bin/bash

:<<!
******NOTICE******
MUST set SDK_PATH & BIN_PATH firstly!!!
example:
export SDK_PATH=~/esp_iot_sdk_freertos
export BIN_PATH=~/esp8266_bin
!

export SDK_PATH=$SDK_PATH
export BIN_PATH=$BIN_PATH
make BOOT=0 APP=0 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=6
