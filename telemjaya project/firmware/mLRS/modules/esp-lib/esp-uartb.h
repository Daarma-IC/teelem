//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// ESP UARTB
//********************************************************
// For ESP32:
// usefull resource
// - https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html
//********************************************************
#ifndef ESPLIB_UARTB_H
#define ESPLIB_UARTB_H

#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
volatile uint32_t telemjaya_uartb_rx_total = 0;
#endif

#ifdef TELEMJAYA_MAVLINK_SELFTEST
// Internal MAVLink source used to validate Air -> RF -> Ground -> GCS without
// opening Air's CH9102 port (opening that port resets this LilyGO revision).
static uint8_t telemjaya_test_frame[17];
static uint8_t telemjaya_test_pos = 0;
static uint8_t telemjaya_test_len = 0;
static uint8_t telemjaya_test_seq = 0;
static uint32_t telemjaya_test_last_ms = 0;

static uint16_t telemjaya_test_crc_accumulate(uint8_t data, uint16_t crc)
{
    uint8_t tmp = data ^ (uint8_t)(crc & 0xFF);
    tmp ^= (tmp << 4);
    return (crc >> 8) ^ ((uint16_t)tmp << 8) ^ ((uint16_t)tmp << 3) ^ (tmp >> 4);
}

static void telemjaya_test_prepare_heartbeat(void)
{
    if (telemjaya_test_pos < telemjaya_test_len) return;
    uint32_t now = millis();
    if ((uint32_t)(now - telemjaya_test_last_ms) < 1000U) return;
    telemjaya_test_last_ms = now;

    uint8_t* f = telemjaya_test_frame;
    f[0] = 0xFE; // MAVLink v1
    f[1] = 9;
    f[2] = telemjaya_test_seq++;
    f[3] = 1; // vehicle system id
    f[4] = 1; // autopilot component id
    f[5] = 0; // HEARTBEAT message id
    f[6] = f[7] = f[8] = f[9] = 0; // custom_mode
    f[10] = 2;  // MAV_TYPE_QUADROTOR
    f[11] = 3;  // MAV_AUTOPILOT_ARDUPILOTMEGA
    f[12] = 81; // custom mode + stabilize + manual input
    f[13] = 3;  // MAV_STATE_STANDBY
    f[14] = 3;  // MAVLink version

    uint16_t crc = 0xFFFF;
    for (uint8_t i = 1; i <= 14; i++) crc = telemjaya_test_crc_accumulate(f[i], crc);
    crc = telemjaya_test_crc_accumulate(50, crc); // HEARTBEAT CRC extra
    f[15] = (uint8_t)(crc & 0xFF);
    f[16] = (uint8_t)(crc >> 8);
    telemjaya_test_pos = 0;
    telemjaya_test_len = sizeof(telemjaya_test_frame);
}
#endif

#if defined TELEMJAYA_ARDUINO_IDE_BUILD && defined ESP32 && defined UARTB_USE_SERIAL2
  #include <driver/uart.h>
  #define TELEMJAYA_UARTB_NATIVE_IDF
  static const uart_port_t telemjaya_uartb_num = UART_NUM_2;
  static bool telemjaya_uartb_installed = false;
#endif


//-------------------------------------------------------
// Enums
//-------------------------------------------------------
#ifndef ESPLIB_UART_ENUMS
#define ESPLIB_UART_ENUMS

typedef enum {
    XUART_PARITY_NO = 0,
    XUART_PARITY_EVEN,
    XUART_PARITY_ODD,
} UARTPARITYENUM;

typedef enum {
//    UART_STOPBIT_0_5 = 0, // not supported by ESP
    UART_STOPBIT_1 = 0,
    UART_STOPBIT_2,
} UARTSTOPBITENUM;

#endif


//-------------------------------------------------------
// Defines
//-------------------------------------------------------

#ifdef UARTB_USE_SERIAL
  #define UARTB_SERIAL_NO       Serial
#elif defined UARTB_USE_SERIAL1
  #define UARTB_SERIAL_NO       Serial1
#elif defined UARTB_USE_SERIAL2
  #define UARTB_SERIAL_NO       Serial2
#else
  #error UARTB_SERIAL_NO must be defined!
#endif

#ifndef UARTB_TXBUFSIZE
  #define UARTB_TXBUFSIZE       256 // MUST be 2^N
#endif
#ifndef UARTB_RXBUFSIZE
  #define UARTB_RXBUFSIZE       256 // MUST be 2^N
#endif

#ifdef ESP32
  #if (UARTB_TXBUFSIZE > 0) && (UARTB_TXBUFSIZE < 256)
    #error UARTB_TXBUFSIZE must be 0 or >= 256
  #endif
  #if (UARTB_RXBUFSIZE > 0) && (UARTB_RXBUFSIZE < 256)
    #error UARTB_TXBUFSIZE must be 0 or >= 256
  #endif
#endif


//-------------------------------------------------------
// TX routines
//-------------------------------------------------------

IRAM_ATTR void uartb_putbuf(uint8_t* buf, uint16_t len)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    if (telemjaya_uartb_installed) uart_write_bytes(telemjaya_uartb_num, buf, len);
#else
    UARTB_SERIAL_NO.write((uint8_t*)buf, len);
#endif
}


IRAM_ATTR uint16_t uartb_tx_notfull(void)
{
    return 1; // fifo not full
}


IRAM_ATTR void uartb_tx_flush(void)
{
#ifdef ESP32
    // flush of tx buffer not available
#elif defined ESP8266
    UARTB_SERIAL_NO.flush();
#endif
}


//-------------------------------------------------------
// RX routines
//-------------------------------------------------------

IRAM_ATTR char uartb_getc(void)
{
#ifdef TELEMJAYA_MAVLINK_SELFTEST
    telemjaya_test_prepare_heartbeat();
    if (telemjaya_test_pos < telemjaya_test_len) {
        uint8_t c = telemjaya_test_frame[telemjaya_test_pos++];
        telemjaya_uartb_rx_total++;
        return (char)c;
    }
    return 0;
#else
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    uint8_t c = 0;
    if (telemjaya_uartb_installed && uart_read_bytes(telemjaya_uartb_num, &c, 1, 0) == 1) {
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
        telemjaya_uartb_rx_total++;
#endif
        return (char)c;
    }
    return 0;
#else
    int value = UARTB_SERIAL_NO.read();
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
    if (value >= 0) telemjaya_uartb_rx_total++;
#endif
    return (value >= 0) ? (char)value : 0;
#endif
#endif
}

IRAM_ATTR void uartb_getbuf(char* buf, uint16_t len)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    if (telemjaya_uartb_installed) uart_read_bytes(telemjaya_uartb_num, buf, len, 0);
#else
    UARTB_SERIAL_NO.readBytes(buf, len);
#endif
}


IRAM_ATTR void uartb_rx_flush(void)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    if (telemjaya_uartb_installed) uart_flush_input(telemjaya_uartb_num);
#else
    while (UARTB_SERIAL_NO.available() > 0) UARTB_SERIAL_NO.read();
#endif
}


IRAM_ATTR uint16_t uartb_rx_bytesavailable(void)
{
#ifdef TELEMJAYA_MAVLINK_SELFTEST
    telemjaya_test_prepare_heartbeat();
    return telemjaya_test_len - telemjaya_test_pos;
#else
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    size_t available = 0;
    if (telemjaya_uartb_installed) uart_get_buffered_data_len(telemjaya_uartb_num, &available);
    return (available > 65535U) ? 65535U : (uint16_t)available;
#else
    int available = UARTB_SERIAL_NO.available();
    return (available > 0) ? available : 0;
#endif
#endif
}


IRAM_ATTR uint16_t uartb_rx_available(void)
{
#ifdef TELEMJAYA_MAVLINK_SELFTEST
    return uartb_rx_bytesavailable() > 0 ? 1 : 0;
#else
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    return uartb_rx_bytesavailable() > 0 ? 1 : 0;
#else
    return (UARTB_SERIAL_NO.available() > 0) ? 1 : 0;
#endif
#endif
}


//-------------------------------------------------------
// INIT routines
//-------------------------------------------------------
// Note: ESP32 has a hardware fifo for tx, which is 128 bytes in size. However, MAVLink messages
// can be larger than this, and data would thus be lost when put only into the fifo. It is therefore
// crucial to set a Tx buffer size of sufficient size. setTxBufferSize() is not available for ESP82xx.

void _uartb_initit(uint32_t baud, UARTPARITYENUM parity, UARTSTOPBITENUM stopbits)
{
#ifdef ESP32
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    uart_config_t cfg = {};
    cfg.baud_rate = baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = (parity == XUART_PARITY_EVEN) ? UART_PARITY_EVEN :
                 (parity == XUART_PARITY_ODD) ? UART_PARITY_ODD : UART_PARITY_DISABLE;
    cfg.stop_bits = (stopbits == UART_STOPBIT_2) ? UART_STOP_BITS_2 : UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.rx_flow_ctrl_thresh = 0;
    cfg.source_clk = UART_SCLK_DEFAULT;
    uart_param_config(telemjaya_uartb_num, &cfg);
    uart_set_pin(telemjaya_uartb_num, UARTB_USE_TX_IO, UARTB_USE_RX_IO,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (!telemjaya_uartb_installed) {
        if (uart_driver_install(telemjaya_uartb_num, UARTB_RXBUFSIZE,
                                UARTB_TXBUFSIZE, 0, nullptr, 0) == ESP_OK) {
            telemjaya_uartb_installed = true;
        }
    }
#else
    UARTB_SERIAL_NO.setTxBufferSize(UARTB_TXBUFSIZE);
    UARTB_SERIAL_NO.setRxBufferSize(UARTB_RXBUFSIZE);

    uint32_t config = SERIAL_8N1;
    switch (parity) {
        case XUART_PARITY_NO:
            switch (stopbits) {
                case UART_STOPBIT_1: config = SERIAL_8N1; break;
                case UART_STOPBIT_2: config = SERIAL_8N2; break;
            }
            break;
        case XUART_PARITY_EVEN:
            switch (stopbits) {
                case UART_STOPBIT_1: config = SERIAL_8E1; break;
                case UART_STOPBIT_2: config = SERIAL_8E2; break;
            }
            break;
        case XUART_PARITY_ODD:
            switch (stopbits) {
                case UART_STOPBIT_1: config = SERIAL_8O1; break;
                case UART_STOPBIT_2: config = SERIAL_8O2; break;
            }
            break;
    }
#if defined UARTB_USE_TX_IO || defined UARTB_USE_RX_IO // both need to be defined
    UARTB_SERIAL_NO.begin(baud, config, UARTB_USE_RX_IO, UARTB_USE_TX_IO);
#else
    UARTB_SERIAL_NO.begin(baud, config);
#endif

    UARTB_SERIAL_NO.setRxFIFOFull(8);  // > 57600 baud sets to 120 which is too much, buffer only 128 bytes
    UARTB_SERIAL_NO.setRxTimeout(1);   // wait for 1 symbol (~11 bits) to trigger Rx ISR, default 2
#endif

#elif defined ESP8266
    UARTB_SERIAL_NO.setRxBufferSize(UARTB_RXBUFSIZE);
    UARTB_SERIAL_NO.begin(baud);
#endif
}


void uartb_setbaudrate(uint32_t baud)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    if (telemjaya_uartb_installed) uart_set_baudrate(telemjaya_uartb_num, baud);
    else _uartb_initit(baud, XUART_PARITY_NO, UART_STOPBIT_1);
#else
    UARTB_SERIAL_NO.end();
    _uartb_initit(baud, XUART_PARITY_NO, UART_STOPBIT_1);
#endif
}


void uartb_setprotocol(uint32_t baud, UARTPARITYENUM parity, UARTSTOPBITENUM stopbits)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    if (telemjaya_uartb_installed) uart_set_baudrate(telemjaya_uartb_num, baud);
    else _uartb_initit(baud, parity, stopbits);
#else
    UARTB_SERIAL_NO.end();
    _uartb_initit(baud, parity, stopbits);
#endif
}


void uartb_init_isroff(void)
{
#ifdef TELEMJAYA_UARTB_NATIVE_IDF
    _uartb_initit(UARTB_BAUD, XUART_PARITY_NO, UART_STOPBIT_1);
#else
    UARTB_SERIAL_NO.end();
    _uartb_initit(UARTB_BAUD, XUART_PARITY_NO, UART_STOPBIT_1);
#endif
}


void uartb_init(void)
{
    uartb_init_isroff();
    // isr is enabled !
}

void uartb_rx_enableisr(FunctionalState flag)
{
    // not supported on ESP, allows in functionality without lots of ifdefs
}


//-------------------------------------------------------
// System bootloader
//-------------------------------------------------------
// ESP8266, ESP32 can't reboot into system bootloader

IRAM_ATTR uint8_t uartb_has_systemboot(void)
{
    return 0;
}


#endif // ESPLIB_UARTB_H
