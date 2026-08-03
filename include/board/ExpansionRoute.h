#pragma once

#include <cstdint>

#include "board/BoardProfile.h"

enum class PeripheralId : uint8_t {
    Cc1101,
    Nrf24,
    Pn532,
    IrTx,
    IrRx,
    Rf433Tx,
    Rf433Rx,
    Gps,
    Sd,
};

enum class Transport : uint8_t {
    None,
    Spi,
    I2c,
    Uart,
    Gpio,
};

enum class BusId : uint8_t {
    None,
    SharedSpi,
    I2c0,
    Uart0,
    Gpio,
};

struct ExpansionRoute {
    PeripheralId peripheral;
    BoardId board;
    Transport transport;
    BusId bus;
    int cs;
    int controlA;
    int controlB;
    uint8_t mutexGroup;
    const char* wiringHint;
};

const ExpansionRoute& findRoute(BoardId board, PeripheralId peripheral);
