#ifndef TELEMJAYA_OLED_H
#define TELEMJAYA_OLED_H
#pragma once

#ifdef TELEMJAYA_HAS_OLED

#include <stdio.h>
#include "thirdparty/gdisp.h"

#if defined(DEVICE_IS_RECEIVER) && defined(TELEMJAYA_ARDUINO_IDE_BUILD)
extern volatile uint32_t telemjaya_uartb_rx_total;
#endif

extern "C" void gdisp_setpixel_(uint16_t x, uint16_t y, uint16_t color);
extern tGDisplay gdisp;

#if defined(DEVICE_IS_TRANSMITTER) || defined(TELEMJAYA_ARDUINO_IDE_BUILD)
// The photographed Ground unit renders the SSD1306 framebuffer as a horizontal
// mirror. Mirror the drawing coordinates in software to cancel that panel
// orientation. Arduino-ESP32 3.x needs the same correction on the Air sketch.
static void telemjaya_mirrored_setpixel(uint16_t x, uint16_t y, uint16_t color)
{
    gdisp_setpixel_((GDISPLAY_COLUMNS - 1) - x, y, color);
}
#endif

class tTelemJayaOled
{
  public:
    void Init(void)
    {
        tick_ms = 0;
        if (initialized) {
            DrawBoot();
            return;
        }
        i2c_init();
        i2c_setdeviceadr(SSD1306_ADR);
        gdisp_init(GDISPLAY_TYPE_SSD1306);
        // LilyGO T3 V1.6.1 panel is horizontally mirrored with the
        // upstream gdisp default (A1). A0 restores readable orientation.
        uint8_t segment_remap_normal = 0xA0;
        i2c_put_blocked(SSD1306_CMD, &segment_remap_normal, 1);
#if defined(DEVICE_IS_TRANSMITTER) || defined(TELEMJAYA_ARDUINO_IDE_BUILD)
        gdisp.setpixel_ptr = &telemjaya_mirrored_setpixel;
#endif
        initialized = true;
        DrawBoot();
        WaitForUpdate();
    }

    void ShowRadioError(uint8_t version = 0)
    {
        if (!initialized) return;
        gdisp_clear();
#ifdef DEVICE_IS_RECEIVER
        PutLine(0, "TelemJaya AIR");
#else
        PutLine(0, "TelemJaya GROUND");
#endif
        PutLine(2, "RADIO ERROR");
        PutLine(3, "SX1276 NOT FOUND");
        char line[24];
        snprintf(line, sizeof(line), "REG 0x%02X NEED 12", version);
        PutLine(4, line);
        PutLine(5, "CHECK SPI/PINS");
        gdisp_update();
    }

    void ShowBootStage(const char* stage)
    {
        if (!initialized) return;
        WaitForUpdate();
        gdisp_clear();
#ifdef DEVICE_IS_RECEIVER
        PutLine(0, "TelemJaya AIR");
#else
        PutLine(0, "TelemJaya GROUND");
#endif
#ifdef TELEMJAYA_FW_VERSION
        PutLine(1, "FW " TELEMJAYA_FW_VERSION);
#endif
        PutLine(2, "BOOTING...");
        PutLine(3, stage);
        PutLine(5, "OLED: OK");
        gdisp_update();
        WaitForUpdate();
    }

    void Tick_ms(void)
    {
        if (!initialized) return;
        if (++tick_ms < 500) return;
        tick_ms = 0;
        if (!gdisp_update_completed()) return;
        Draw();
    }

  private:
    bool initialized = false;
    uint16_t tick_ms;
#if defined(DEVICE_IS_RECEIVER) && defined(TELEMJAYA_ARDUINO_IDE_BUILD)
    uint32_t uart_rx_last = 0;
#endif

    void WaitForUpdate(void)
    {
        while (!gdisp_update_completed()) {
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
            // Air's mLRS millisecond clock is not running during early boot.
            // Arduino delay yields independently and also feeds the watchdog.
            delay(1);
#else
            delay_ms(1);
#endif
        }
    }

    const char* ModeText(void)
    {
        switch (Config.Mode) {
        case MODE_50HZ: return "50Hz";
        case MODE_31HZ: return "31Hz";
        case MODE_19HZ: return "19Hz";
        case MODE_FLRC_111HZ: return "FLRC111";
        case MODE_FSK_50HZ: return "FSK50";
        case MODE_19HZ_7X: return "19Hz7x";
        default: return "unknown";
        }
    }

    const char* BandText(void)
    {
        switch (Config.FrequencyBand) {
        case SETUP_FREQUENCY_BAND_915_MHZ_FCC: return "915";
        case SETUP_FREQUENCY_BAND_868_MHZ: return "868";
        case SETUP_FREQUENCY_BAND_433_MHZ: return "433";
        case SETUP_FREQUENCY_BAND_866_MHZ_IN: return "866";
        default: return "RF";
        }
    }

    void PutLine(uint8_t row, const char* text)
    {
        gdisp_setcurXY(0, row * 10);
        gdisp_puts(text);
    }

    void DrawBoot(void)
    {
        gdisp_clear();
#ifdef DEVICE_IS_RECEIVER
        PutLine(0, "TelemJaya AIR");
#else
        PutLine(0, "TelemJaya GROUND");
#endif
#ifdef TELEMJAYA_FW_VERSION
        PutLine(1, "FW " TELEMJAYA_FW_VERSION);
#endif
        PutLine(2, "BOOTING...");
        PutLine(3, "RADIO CHECK");
        PutLine(5, "OLED: OK");
        gdisp_update();
    }

    void Draw(void)
    {
        char line[32];
        gdisp_clear();

#ifdef DEVICE_IS_RECEIVER
        PutLine(0, "TelemJaya AIR");
#else
        PutLine(0, "TelemJaya GROUND");
#endif

        if (connected() && !bind.IsInBind()) {
            PutLine(1, "[ CONNECTED ]");
        } else if (bind.IsInBind()) {
            PutLine(1, "[ BINDING ]");
        } else {
            snprintf(line, sizeof(line), "[ SEARCH ] %us", bind.AutoBindRemainingSeconds());
            PutLine(1, line);
        }

        snprintf(line, sizeof(line), "RF %s %sM", ModeText(), BandText());
        PutLine(2, line);

        if (bind.IsInBind()) {
            snprintf(line, sizeof(line), "TX%lu RX%lu",
                     (unsigned long)bind.BindTxCount(),
                     (unsigned long)bind.BindRxCount());
            PutLine(3, line);
            snprintf(line, sizeof(line), "BAD %lu",
                     (unsigned long)bind.BindBadCount());
            PutLine(4, line);
            PutLine(5, "BOOT: SHORT PRESS");
            gdisp_update();
            return;
        }

#ifdef DEVICE_IS_RECEIVER
        uint8_t lq = connected() ? stats.GetLQ_rc() : 0;
#else
        uint8_t lq = connected() ? stats.GetReceivedLQ_rc() : 0;
#endif
        int8_t rssi = connected() ? stats.GetLastRssi() : 0;
        if (connected()) {
            snprintf(line, sizeof(line), "LQ   %u%%", lq);
        } else {
            snprintf(line, sizeof(line), "LQ   --");
        }
        PutLine(3, line);

        if (connected()) {
            snprintf(line, sizeof(line), "RSSI %d dBm", rssi);
        } else {
            snprintf(line, sizeof(line), "RSSI --");
        }
        PutLine(4, line);

#ifdef DEVICE_IS_RECEIVER
#ifdef TELEMJAYA_ARDUINO_IDE_BUILD
        uint32_t uart_now = telemjaya_uartb_rx_total;
        uint32_t uart_bps = (uart_now - uart_rx_last) * 2U;
        uart_rx_last = uart_now;
        snprintf(line, sizeof(line), "M%u%% U%luB/s",
                 connected() ? stats.GetMavlinkLQ() : 0,
                 (unsigned long)uart_bps);
#else
        snprintf(line, sizeof(line), "MAV %u%%  FC57K", connected() ? stats.GetMavlinkLQ() : 0);
#endif
#else
        snprintf(line, sizeof(line), "MAV %u%% USB57K", connected() ? stats.GetMavlinkLQ() : 0);
#endif
        PutLine(5, line);

        gdisp_update();
    }
};

#else

class tTelemJayaOled
{
  public:
    void Init(void) {}
    void Tick_ms(void) {}
    void ShowRadioError(uint8_t = 0) {}
    void ShowBootStage(const char*) {}
};

#endif
#endif
