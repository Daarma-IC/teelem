#ifndef TELEMJAYA_CONSOLE_H
#define TELEMJAYA_CONSOLE_H
#pragma once

// Air has a dedicated USB UART0 because its flight-controller link uses
// Serial2 on GPIO16/17. Ground cannot emit text on UART0 without corrupting
// the MAVLink stream to Mission Planner, so the production console is Air-only.
#if defined(TELEMJAYA_ARDUINO_IDE_BUILD) && defined(DEVICE_IS_RECEIVER) && defined(ESP32) && !defined(TELEMJAYA_FC_UART0)

#include <Arduino.h>

class tTelemJayaConsole
{
  public:
    void Init(void)
    {
        tick_ms = 0;
        Serial.begin(115200);
        delay(50);
        Serial.println();
        Serial.println("TELEMJAYA AIR STATUS CONSOLE v0.4.0");
        Serial.println("USB console=115200 | FC UART2=57600 RX34 TX4");
    }

    void Tick_ms(void)
    {
        if (++tick_ms < 1000) return;
        tick_ms = 0;

        const char* state = "SEARCH";
        if (bind.IsInBind()) state = "BINDING";
        if (connected() && !bind.IsInBind()) state = "CONNECTED";

        Serial.printf(
            "TJ AIR | state=%s | bind TX=%lu RX=%lu BAD=%lu | "
            "LQ=%u RSSI=%d MAV=%u%% | FC>GND=%lu B/s GND>FC=%lu B/s\n",
            state,
            (unsigned long)bind.BindTxCount(),
            (unsigned long)bind.BindRxCount(),
            (unsigned long)bind.BindBadCount(),
            connected() ? stats.GetLQ_rc() : 0,
            connected() ? stats.GetLastRssi() : 0,
            connected() ? stats.GetMavlinkLQ() : 0,
            (unsigned long)stats.bytes_transmitted.GetBytesPerSec(),
            (unsigned long)stats.bytes_received.GetBytesPerSec());
    }

  private:
    uint16_t tick_ms = 0;
};

#else

class tTelemJayaConsole
{
  public:
    void Init(void) {}
    void Tick_ms(void) {}
};

#endif

#endif
