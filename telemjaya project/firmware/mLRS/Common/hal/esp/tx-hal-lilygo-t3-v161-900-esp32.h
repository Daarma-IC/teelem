//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// hal
//*******************************************************

//-------------------------------------------------------
// LilyGO T3 V1.6.1, ESP32-PICO-D4 + SX1276, ground unit
//-------------------------------------------------------

#define DEVICE_HAS_SINGLE_LED
#define TELEMJAYA_AUTO_BIND_AFTER_POWERUP
#define TELEMJAYA_HAS_OLED
#define TELEMJAYA_POLL_RADIO_IRQ
#define DEVICE_HAS_NO_COM
#define DEVICE_HAS_NO_DEBUG


//-- UARTS
// UARTB reaches Mission Planner through the onboard CH9102 USB-UART bridge.

#define UARTB_USE_SERIAL
#define UARTB_BAUD                TX_SERIAL_BAUDRATE
#define UARTB_USE_TX_IO           IO_P1
#define UARTB_USE_RX_IO           IO_P3
#define UARTB_TXBUFSIZE           TX_SERIAL_TXBUFSIZE
#define UARTB_RXBUFSIZE           TX_SERIAL_RXBUFSIZE

#define UARTF_USE_SERIAL
#define UARTF_BAUD                115200


//-- SX1276 & SPI

#define SPI_CS_IO                 IO_P18
#define SPI_MISO                  IO_P19
#define SPI_MOSI                  IO_P27
#define SPI_SCK                   IO_P5
#define SPI_FREQUENCY             1000000L
#define SX_RESET                  IO_P23
#define SX_DIO                    IO_P26 // SX1276 DIO0

IRQHANDLER(void SX_DIO_EXTI_IRQHandler(void);)

void sx_init_gpio(void)
{
    gpio_init(SX_RESET, IO_MODE_OUTPUT_PP_HIGH);
    gpio_init(SX_DIO, IO_MODE_INPUT_ANALOG);
}

IRAM_ATTR void sx_amp_transmit(void) {}
IRAM_ATTR void sx_amp_receive(void) {}

void sx_dio_init_exti_isroff(void) {}
// TelemJaya polls the latched SX1276 IRQ flags in the main loop. This avoids
// SPI transactions inside an ESP32 GPIO ISR, which is unreliable on newer
// Arduino-ESP32 cores.
void sx_dio_enable_exti_isr(void) {}
IRAM_ATTR void sx_dio_exti_isr_clearflag(void) {}


//-- BOOT button: hold after normal startup to request binding.

#define BUTTON                     IO_P0
void button_init(void) { gpio_init(BUTTON, IO_MODE_INPUT_PU); }
IRAM_ATTR bool button_pressed(void) { return gpio_read_activelow(BUTTON) ? true : false; }


//-- LED (active high)

#define LED_RED                   IO_P25


//-- Onboard SSD1306 OLED

#define I2C_SDA_IO                IO_P21
#define I2C_SCL_IO                IO_P22
#define I2C_CLOCKSPEED            800000L

void leds_init(void)
{
    gpio_init(LED_RED, IO_MODE_OUTPUT_PP_LOW);
}

IRAM_ATTR void led_red_off(void) { gpio_low(LED_RED); }
IRAM_ATTR void led_red_on(void) { gpio_high(LED_RED); }
IRAM_ATTR void led_red_toggle(void) { gpio_toggle(LED_RED); }


//-- POWER (bare SX1276, no external PA/LNA control)

#define POWER_GAIN_DBM            0
#define POWER_SX1276_MAX          SX1276_OUTPUT_POWER_MAX
#define POWER_USE_DEFAULT_RFPOWER_CALC

#define RFPOWER_DEFAULT           1

const rfpower_t rfpower_list[] = {
    { .dbm = POWER_0_DBM, .mW = 1 },
    { .dbm = POWER_10_DBM, .mW = 10 },
    { .dbm = POWER_17_DBM, .mW = 50 },
};
