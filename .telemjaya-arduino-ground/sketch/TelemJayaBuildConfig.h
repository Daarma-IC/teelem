#line 1 "D:\\aaa\\asambly\\telemjaya project\\arduino\\TelemJaya_Ground\\TelemJayaBuildConfig.h"
#pragma once

#if !defined(ARDUINO_ARCH_ESP32)
#error "Select an ESP32 board before compiling TelemJaya Ground."
#endif

#define TX_LILYGO_T3_V161_900_ESP32

// Arduino-ESP32 3.x no longer pulls this register declaration indirectly.
#include <soc/gpio_struct.h>

#ifndef CONFIG_DISABLE_HAL_LOCKS
#define CONFIG_DISABLE_HAL_LOCKS 1
#endif
#ifndef CONFIG_GPIO_CTRL_FUNC_IN_IRAM
#define CONFIG_GPIO_CTRL_FUNC_IN_IRAM 1
#endif
#ifndef CONFIG_GPTIMER_CTRL_FUNC_IN_IRAM
#define CONFIG_GPTIMER_CTRL_FUNC_IN_IRAM 1
#endif
#ifndef CONFIG_GPTIMER_ISR_IRAM_SAFE
#define CONFIG_GPTIMER_ISR_IRAM_SAFE 1
#endif
#ifndef CONFIG_SPI_MASTER_IN_IRAM
#define CONFIG_SPI_MASTER_IN_IRAM 1
#endif
#ifndef CONFIG_UART_ISR_IN_IRAM
#define CONFIG_UART_ISR_IN_IRAM 1
#endif
#ifndef CONFIG_I2C_ISR_IRAM_SAFE
#define CONFIG_I2C_ISR_IRAM_SAFE 1
#endif
