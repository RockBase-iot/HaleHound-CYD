#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

// =============================================================================
// SPANDECK SD Card Firmware Update
// I flash new firmware.bin from my SD card — no laptop needed
// My pattern: same as my serial_monitor.cpp / gps_module.cpp
// Created: 2026-02-15
// =============================================================================

#include <Arduino.h>
#include "services/FirmwareImageValidator.h"

// Main entry point - called from Tools > Update Firmware
void firmwareUpdateScreen();

#endif // FIRMWARE_UPDATE_H
