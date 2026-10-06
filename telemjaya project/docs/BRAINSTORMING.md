# Brainstorming dan Keputusan Arsitektur

## Tujuan

Membuat modem telemetri dua arah yang dapat digunakan oleh ArduPlane,
ArduCopter, dan vehicle ArduPilot lain tanpa firmware khusus per jenis vehicle.
Mission Planner harus melihat modem ground sebagai COM port MAVLink biasa.

## Arsitektur yang dipilih

```text
sensor
  -> driver dan estimator ArduPilot
  -> MAVLink pada port TELEM
  -> UART2 LilyGO Air
  -> mLRS/MAVLinkX melalui SX1276
  -> LilyGO Ground
  -> UART0 dan CH9102 USB
  -> Mission Planner
```

Jalur balik menggunakan rantai yang sama dengan arah terbalik. Karena itu
parameter, mission, command, mode change, dan message request dapat mencapai
ArduPilot; link ini bukan telemetry satu arah.

## Hardware target

- Board: LilyGO T3 V1.6.1.
- MCU: ESP32-PICO-D4, flash 4 MB.
- Radio: SX1276, versi non-TCXO.
- RF hardware range: 830-945 MHz.
- Mode mLRS: `19 Hz 7x`, karena SX1276.
- Daya awal dibatasi ke pilihan 0, 10, dan 17 dBm tanpa asumsi external PA.

Pin radio:

| Signal | GPIO |
|---|---:|
| SCK | 5 |
| MISO | 19 |
| MOSI | 27 |
| CS/NSS | 18 |
| RESET | 23 |
| DIO0/IRQ | 26 |
| DIO1 | 33 |
| DIO2 | 32 |
| LED | 25 |

## Pembagian role

### Air unit

- Build environment: `rx-lilygo-t3-v161-900`.
- mLRS role: receiver, yaitu perangkat di vehicle.
- Flight-controller UART: ESP32 `Serial2`.
- ESP32 RX: GPIO3, menerima data dari FC TX.
- ESP32 TX: GPIO1, mengirim data menuju FC RX.
- UART0 tetap bebas untuk flashing dan diagnosis.

### Ground unit

- Build environment: `tx-lilygo-t3-v161-900`.
- mLRS role: transmitter, yaitu perangkat di GCS.
- MAVLink memakai UART0 GPIO1/3 menuju bridge USB CH9102 onboard.
- Mission Planner membuka COM port board secara langsung.

## Keputusan protokol

- Firmware tidak membaca I2C/SPI sensor Pixhawk secara langsung.
- ArduPilot tetap menjadi pemilik sensor, calibration, filtering, dan EKF.
- TelemJaya mengangkut byte stream MAVLink dua arah.
- MAVLink 2 dipakai pada port TELEM.
- Baud awal 57600 untuk integrasi pertama.
- MAVLinkX mLRS dapat dipakai untuk kompresi dan optimasi link.

## Kapasitas dan batasan

SX1276 `19 Hz 7x` tidak cukup untuk menyalurkan setiap raw IMU sample pada
ratusan Hz. Data penerbangan normal, parameter, mission, command, GPS,
attitude, battery, EKF, dan status dapat diangkut dengan stream rate yang
wajar. Raw sensor berkecepatan tinggi tetap direkam ke DataFlash log.

Tabel mLRS saat ini menggunakan frequency plan `915 MHz FCC` dan melakukan
frequency hopping; board yang dijual sebagai 923 MHz tidak berarti link diam
pada satu frekuensi 923 MHz. Frequency plan final wajib diaudit terhadap
peraturan wilayah operasi sebelum pemakaian lapangan.

## Risiko yang harus diuji di hardware

1. Memastikan board adalah SX1276 non-TCXO. Varian TCXO memakai GPIO33 untuk
   enable oscillator dan membutuhkan HAL berbeda.
2. Memastikan output RF memakai jalur PA_BOOST yang sesuai dengan driver mLRS.
3. Menguji reset polarity, DIO0 interrupt, RSSI, packet loss, dan output power.
4. Menguji UART bidirectional, parameter download, mission upload, serta
   command acknowledgement.
5. Menguji stream rate supaya antrean serial tidak melebihi kapasitas RF.
