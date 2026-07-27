#ifndef SERIAL_MONITOR_H
#define SERIAL_MONITOR_H

// =============================================================================
// HaleHound-CYD UART Serial Terminal
// Hardware UART passthrough for target device debug ports
// No laptop needed - read router/IoT/camera UART output on CYD screen
// Created: 2026-02-15
// =============================================================================

#include <Arduino.h>

// Pin mode selection
enum UARTPinMode {
    UART_PIN_P1,       // P1 JST connector: GPIO3 RX, GPIO1 TX (full duplex)
    UART_PIN_SPEAKER   // Speaker connector: GPIO26 RX only
};

// Main entry point - called from Tools > Serial Monitor
void serialMonitorScreen();

#endif // SERIAL_MONITOR_H
