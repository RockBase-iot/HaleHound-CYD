#include "board/ExpansionRoute.h"

namespace {
constexpr ExpansionRoute kRoutes[] = {
    {PeripheralId::Cc1101, BoardId::Esp32Cyd, Transport::Spi, BusId::SharedSpi, 27, 22, 35, 1, "CC1101 CS/GDO0/GDO2"},
    {PeripheralId::Nrf24, BoardId::Esp32Cyd, Transport::Spi, BusId::SharedSpi, 4, 16, -1, 1, "nRF24 CSN/CE"},
    {PeripheralId::Pn532, BoardId::Esp32Cyd, Transport::I2c, BusId::I2c0, -1, -1, -1, 0, "PN532 I2C board-default SDA/SCL"},
    {PeripheralId::IrTx, BoardId::Esp32Cyd, Transport::Gpio, BusId::Gpio, -1, 26, -1, 2, "IR TX GPIO"},
    {PeripheralId::IrRx, BoardId::Esp32Cyd, Transport::Gpio, BusId::Gpio, -1, 34, -1, 2, "IR RX GPIO"},
    {PeripheralId::Rf433Tx, BoardId::Esp32Cyd, Transport::Gpio, BusId::Gpio, -1, 26, -1, 2, "RF433 TX GPIO"},
    {PeripheralId::Rf433Rx, BoardId::Esp32Cyd, Transport::Gpio, BusId::Gpio, -1, 34, -1, 2, "RF433 RX GPIO"},
    {PeripheralId::Gps, BoardId::Esp32Cyd, Transport::Uart, BusId::Uart0, -1, 26, 17, 0, "GPS RX/TX"},
    {PeripheralId::Sd, BoardId::Esp32Cyd, Transport::Spi, BusId::SharedSpi, 5, -1, -1, 1, "SD CS"},
    {PeripheralId::Cc1101, BoardId::NmCydC5, Transport::Spi, BusId::SharedSpi, 9, 8, -1, 1, "C5 CC1101 CS/GDO0"},
    {PeripheralId::Nrf24, BoardId::NmCydC5, Transport::Spi, BusId::SharedSpi, 9, 8, -1, 1, "C5 nRF24 CSN/CE"},
    {PeripheralId::Pn532, BoardId::NmCydC5, Transport::I2c, BusId::I2c0, -1, 4, 5, 0, "PN532 SDA/SCL"},
    {PeripheralId::IrTx, BoardId::NmCydC5, Transport::Gpio, BusId::Gpio, -1, 10, -1, 2, "IR TX GPIO"},
    {PeripheralId::IrRx, BoardId::NmCydC5, Transport::Gpio, BusId::Gpio, -1, 11, -1, 2, "IR RX GPIO"},
    {PeripheralId::Rf433Tx, BoardId::NmCydC5, Transport::Gpio, BusId::Gpio, -1, 10, -1, 2, "RF433 TX GPIO"},
    {PeripheralId::Rf433Rx, BoardId::NmCydC5, Transport::Gpio, BusId::Gpio, -1, 11, -1, 2, "RF433 RX GPIO"},
    {PeripheralId::Gps, BoardId::NmCydC5, Transport::Uart, BusId::Uart0, -1, 3, 4, 0, "GPS RX/TX"},
    {PeripheralId::Sd, BoardId::NmCydC5, Transport::Spi, BusId::SharedSpi, 10, -1, -1, 1, "C5 SD CS"},
};

constexpr ExpansionRoute kMissing{
    PeripheralId::Cc1101,
    BoardId::Esp32Cyd,
    Transport::None,
    BusId::None,
    -1,
    -1,
    -1,
    0,
    "route unavailable",
};
}

const ExpansionRoute& findRoute(BoardId board, PeripheralId peripheral) {
    for (const auto& route : kRoutes) {
        if (route.board == board && route.peripheral == peripheral) {
            return route;
        }
    }
    return kMissing;
}
