//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// ESP SPI Interface
//********************************************************
#ifndef ESPLIB_SPI_H
#define ESPLIB_SPI_H


#include <SPI.h>

#ifdef ESP8266
#define SPI_MSBFIRST  MSBFIRST // for some reasons not defined for EPR82xx
#endif


//-- select functions

#ifdef SPI_CS_IO

#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
static SPISettings telemjaya_spi_settings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0);
#endif

IRAM_ATTR void spi_select(void)
{
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
    // Match the standalone detector's known-good transaction exactly.
    SPI.beginTransaction(telemjaya_spi_settings);
    digitalWrite(SPI_CS_IO, LOW);
#else
    gpio_low(SPI_CS_IO);
#endif
}

IRAM_ATTR void spi_deselect(void)
{
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
    digitalWrite(SPI_CS_IO, HIGH);
    SPI.endTransaction();
#else
    gpio_high(SPI_CS_IO);
#endif
}

#endif // #ifdef SPI_CS_IO


//-- transmit, transfer, read, write functions

// these are blocking
// utilize ESP SPI buffer
// transferBytes()
// - does while(SPI1CMD & SPIBUSY) {} as needed
// - if dataout or datain are not aligned, then it does memcpy to aligned buffer on stack
// - sends 0xFFFFFFFF if no out data!! Problem: sx datasheet says 0x00

IRAM_ATTR void spi_transfer(const uint8_t* dataout, uint8_t* datain, const uint8_t len)
{
#if defined ESP32 && defined TELEMJAYA_ARDUINO_IDE_BUILD
    for (uint8_t i = 0; i < len; i++) {
        uint8_t value = SPI.transfer(dataout ? dataout[i] : 0x00);
        if (datain) datain[i] = value;
    }
#elif defined ESP32
    spiTransferBytesNL(SPI.bus(), dataout, datain, len);
#elif defined ESP8266
    SPI.transferBytes(dataout, datain, len);
#endif
}


IRAM_ATTR void spi_read(uint8_t* datain, const uint8_t len)
{
#if defined ESP32 && defined TELEMJAYA_ARDUINO_IDE_BUILD
    for (uint8_t i = 0; i < len; i++) datain[i] = SPI.transfer(0x00);
#elif defined ESP32
    spiTransferBytesNL(SPI.bus(), nullptr, datain, len);
#elif defined ESP8266
    SPI.transferBytes(nullptr, datain, len);
#endif
}


IRAM_ATTR void spi_write(const uint8_t* dataout, uint8_t len)
{
#if defined ESP32 && defined TELEMJAYA_ARDUINO_IDE_BUILD
    while (len--) SPI.transfer(*dataout++);
#elif defined ESP32
    spiTransferBytesNL(SPI.bus(), dataout, nullptr, len);
#elif defined ESP8266
    SPI.transferBytes(dataout, nullptr, len);
#endif
}


//-------------------------------------------------------
// INIT routines
//-------------------------------------------------------

void spi_setnop(uint8_t nop)
{
    // currently not supported, should be 0x00 fo sx, is 0xFF per ESP SPI library
}


void spi_init(void)
{
#ifdef SPI_CS_IO
    gpio_init(SPI_CS_IO, IO_MODE_OUTPUT_PP_HIGH);
#endif

#if defined ESP32 && defined TELEMJAYA_ARDUINO_IDE_BUILD
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS_IO);
#elif defined ESP32
    spiEndTransaction(SPI.bus());
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_CS_IO);
    spiSimpleTransaction(SPI.bus());
#elif defined ESP8266
    SPI.begin();
#endif
#ifndef TELEMJAYA_ARDUINO_IDE_BUILD
    SPI.setFrequency(SPI_FREQUENCY);
    SPI.setBitOrder(SPI_MSBFIRST);
    SPI.setDataMode(SPI_MODE0);
#endif
}


#endif // ESPLIB_SPI_H
