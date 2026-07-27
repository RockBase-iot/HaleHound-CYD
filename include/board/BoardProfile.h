#pragma once

#include "cyd_config.h"

namespace board {

struct SpiPins {
    int sck;
    int miso;
    int mosi;
};

struct BoardProfile {
    const char* name;
    int screenWidth;
    int screenHeight;
    int backlightPin;
    int touchCsPin;
    int touchIrqPin;
    SpiPins sharedSpi;
    int sdCsPin;
    int cc1101CsPin;
    int nrf24CsnPin;
    int nrf24CePin;
    int pn532CsPin;
    int gpsRxPin;
    int gpsTxPin;
    bool hasAudioAmplifier;
    bool radioChipSelectShared;
    bool pn532UsesI2c;
};

#if defined(CYD_C5)
static constexpr int PN532_CS_PIN = -1;
static constexpr bool PN532_USES_I2C = true;
#else
static constexpr int PN532_CS_PIN = PN532_CS;
static constexpr bool PN532_USES_I2C = false;
#endif

#if defined(NMRF_HAT) || defined(CYD_C5)
static constexpr bool RADIO_CHIP_SELECT_SHARED = true;
#else
static constexpr bool RADIO_CHIP_SELECT_SHARED = false;
#endif

static constexpr BoardProfile CURRENT = {
    CYD_BOARD_NAME,
    CYD_SCREEN_WIDTH,
    CYD_SCREEN_HEIGHT,
    CYD_TFT_BL,
    CYD_TOUCH_CS,
    CYD_TOUCH_IRQ,
    {VSPI_SCK, VSPI_MISO, VSPI_MOSI},
    SD_CS,
    CC1101_CS,
    NRF24_CSN,
    NRF24_CE,
    PN532_CS_PIN,
    GPS_RX_PIN,
    GPS_TX_PIN,
    CYD_HAS_AMP != 0,
    RADIO_CHIP_SELECT_SHARED,
    PN532_USES_I2C,
};

constexpr bool isOptionalGpioValid(int pin) {
    return pin >= -1 && pin <= 48;
}

static_assert(CURRENT.screenWidth > 0 && CURRENT.screenHeight > 0,
              "Board display dimensions must be positive");
static_assert(isOptionalGpioValid(CURRENT.backlightPin) && CURRENT.backlightPin >= 0,
              "Board backlight pin is invalid");
static_assert(isOptionalGpioValid(CURRENT.touchCsPin) && CURRENT.touchCsPin >= 0,
              "Board touch CS pin is invalid");
static_assert(CURRENT.sharedSpi.sck != CURRENT.sharedSpi.miso &&
                  CURRENT.sharedSpi.sck != CURRENT.sharedSpi.mosi &&
                  CURRENT.sharedSpi.miso != CURRENT.sharedSpi.mosi,
              "Shared SPI signal pins must be unique");
static_assert(CURRENT.sdCsPin != CURRENT.cc1101CsPin &&
                  CURRENT.sdCsPin != CURRENT.nrf24CsnPin,
              "SD and radio chip-select pins must be unique");
static_assert(CURRENT.radioChipSelectShared || CURRENT.cc1101CsPin != CURRENT.nrf24CsnPin,
              "CC1101 and NRF24 chip-select pins overlap unexpectedly");
static_assert(CURRENT.pn532UsesI2c ||
                  (CURRENT.pn532CsPin != CURRENT.sdCsPin &&
                   CURRENT.pn532CsPin != CURRENT.cc1101CsPin &&
                   CURRENT.pn532CsPin != CURRENT.nrf24CsnPin),
              "PN532 chip-select overlaps another shared-SPI device");

}  // namespace board