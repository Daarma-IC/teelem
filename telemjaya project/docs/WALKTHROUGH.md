# Walkthrough Pengembangan

Dokumen ini adalah log teknis agar pekerjaan dapat diteruskan bersama tanpa
mengulang discovery dari awal.

## Repository basis

Saat project dibuat, sumber lokal yang diperiksa adalah:

| Repository | Commit | Fungsi |
|---|---|---|
| mLRS | `14ff0b279b032ae6b2e27c08408d3880d9f19e57` | Basis firmware radio |
| ArduPilot | `ecf1a0cc90481e988e7fc95cbd9307759570e19f` | Referensi MAVLink/serial |
| mLRS-hardware | `d63f2f12b5a93140b4f77fca6267176a67bb6045` | Referensi desain hardware |

Ketiga repository upstream di luar folder ini tidak menjadi lokasi perubahan
TelemJaya. Fork kerja firmware berada di `firmware/`.

## Discovery yang sudah dilakukan

1. mLRS sudah mendukung ESP32 dan driver SX1276.
2. Tidak ada target resmi LilyGO T3 V1.6.1 pada daftar HAL mLRS.
3. Penyebutan TTGO Micro32 di mLRS adalah wireless bridge pendamping, bukan
   target radio LilyGO LoRa32 standalone.
4. Target generic ESP32 900 MHz yang ada mengasumsikan true diversity dan pin
   RF board ExpressLRS, sehingga tidak aman dipasang ke LilyGO.
5. Pin T3 V1.6.1 diverifikasi terhadap dokumentasi dan board support LilyGO.
6. ArduPilot mendefinisikan MAVLink dan MAVLink2 sebagai serial protocol 1 dan
   2 di `libraries/AP_SerialManager/AP_SerialManager.h`.

## Perubahan firmware

Target baru yang ditambahkan:

- `firmware/mLRS/Common/hal/esp/rx-hal-lilygo-t3-v161-900-esp32.h`
- `firmware/mLRS/Common/hal/esp/tx-hal-lilygo-t3-v161-900-esp32.h`

Integrasi target ditambahkan ke:

- `firmware/mLRS/Common/hal/esp/esp-device_conf.h`
- `firmware/mLRS/Common/hal/esp/esp-hal.h`
- `firmware/platformio.ini`

Air dan Ground memakai UART0 GPIO1/3. Pada Air menuju FC; pada Ground menuju USB.
Keduanya memakai SPI radio GPIO5/19/27/18, reset GPIO23, dan DIO0 GPIO26.

## Cara build

Dari folder `firmware/`, jalankan:

```powershell
pio run -e rx-lilygo-t3-v161-900
pio run -e tx-lilygo-t3-v161-900
```

Output default yang diharapkan:

```text
firmware/.pio/build/rx-lilygo-t3-v161-900/firmware.bin
firmware/.pio/build/tx-lilygo-t3-v161-900/firmware.bin
```

Setelah berhasil, salin dengan nama yang eksplisit ke `artifacts/`:

```text
telemjaya-air-t3-v161-sx1276.bin
telemjaya-ground-t3-v161-sx1276.bin
```

Pada Windows, toolchain Xtensa lama dapat gagal dengan `CreateProcess` jika
path instalasi PlatformIO terlalu panjang. Build validasi project ini memakai
core path pendek dan build directory tanpa spasi. Ini workaround environment,
bukan perubahan source firmware.

## Hasil build 30 September 2026

Kedua environment selesai dengan status `SUCCESS`:

| Target | RAM | Flash | App binary |
|---|---:|---:|---:|
| Air/RX | 30,192 byte (9.2%) | 345,581 byte (26.4%) | 345,952 byte |
| Ground/TX | 30,968 byte (9.5%) | 357,865 byte (27.3%) | 358,224 byte |

Artefak build terakhir (`0.2.0-bind-led`):

| File | SHA-256 |
|---|---|
| `telemjaya-air-t3-v161-sx1276.bin` | `7DCE2BA1C208DD31E9C62387908CC7B2E2A42D6EAD87BF063AC1331E99767B7F` |
| `telemjaya-ground-t3-v161-sx1276.bin` | `9AB19E8FDF6DCA7915B2ECCC8D42BA3C0D5CB06D4FA8318B0443F5476121E5E3` |
| `esp32-pico-bootloader.bin` | `3D97B71A61331EBADDEF2A74E090C35299650F395F1387F85130010E84E46563` |
| `esp32-pico-partitions.bin` | `148B959CBFF1C38AA8E1D5C0BA9D612C54997B945E56A63F41223EEF650653A1` |
| `esp32-boot-app0.bin` | `F94C5D786A7A8FAB06AC5D10E33BF37711A6697636DC037559EA19CC410A17F0` |

Build membuktikan integrasi source dan HAL dapat di-compile/link. Status ini
belum membuktikan komunikasi RF atau UART pada hardware fisik.

## Layout flash ESP32

Layout yang akan digunakan oleh web flasher:

| Offset | File |
|---:|---|
| `0x1000` | `esp32-pico-bootloader.bin` |
| `0x8000` | `esp32-pico-partitions.bin` |
| `0xE000` | `esp32-boot-app0.bin` |
| `0x10000` | binary Air atau Ground |

Jangan memasang binary Air dan Ground sekaligus pada board yang sama; pilih
salah satu role untuk offset `0x10000`.

## Wiring air unit

| LilyGO Air | Flight controller TELEM |
|---|---|
| GPIO3 RX | TX |
| GPIO1 TX | RX |
| GND | GND |

Jangan menghubungkan output 5 V ke GPIO ESP32. Board dapat diberi supply dari
input yang memang didesain untuk itu; seluruh sinyal UART harus 3,3 V.

## Konfigurasi ArduPilot awal

Pilih nomor serial yang sesuai dengan konektor TELEM board:

```text
SERIALx_PROTOCOL = 2
SERIALx_BAUD = 57
```

Nilai `57` berarti 57600 baud. Restart flight controller setelah perubahan.
Stream rate kemudian diturunkan atau dinaikkan berdasarkan pengukuran link,
bukan diasumsikan semua message dapat dikirim secepat UART.

## Urutan validasi

- [x] Kedua firmware selesai compile dan link.
- [ ] Air dan ground boot tanpa reset loop.
- [ ] SX1276 version register terbaca.
- [ ] Kedua unit bind dan menunjukkan link quality/RSSI.
- [ ] Heartbeat ArduPilot terlihat di ground COM.
- [ ] Mission Planner connect stabil selama minimal 30 menit.
- [ ] Parameter download selesai dan parameter write mendapat respons.
- [ ] Mission upload/download berhasil.
- [ ] Command dan mode change mendapat `COMMAND_ACK`.
- [ ] Telemetry tetap stabil saat armed dengan motor tidak berpropeller.
- [ ] Range test dan failsafe test dilakukan secara terkontrol.
- [ ] Frequency plan dan power telah sesuai wilayah operasi.

## Status saat ini

HAL, environment PlatformIO, generated MAVLink headers, dan binary Air/Ground
sudah tersedia. Binary belum boleh dianggap flight-tested atau dipublikasikan
sebagai stable sebelum bench test dan verifikasi varian non-TCXO selesai.

## Pembaruan binding dan LED 30 September 2026

- GPIO0/BOOT diaktifkan sebagai tombol bind pada target Air dan Ground.
- Tombol BOOT ditahan sekitar 4 detik setelah startup normal untuk meminta bind.
- Kedua role mencari link tersimpan selama 20 detik setelah boot.
- Jika belum connected, kedua role otomatis masuk bind mode setelah 20 detik.
- LED GPIO25: satu kedip panjang = searching, dua kedip cepat = binding, solid = connected.
- Kedua target berhasil dibangun ulang dan binary di `artifacts/` serta `web-flasher/firmware/` diperbarui.

## Pembaruan OLED dan anti-cache

- OLED SSD1306 onboard diaktifkan pada I2C GPIO21/22 untuk Air dan Ground.
- OLED menampilkan state link, countdown auto-bind, RF mode/band, LQ, RSSI, MAVLink LQ, dan UART.
- Transfer display memakai task I2C Core 0 agar loop radio tidak diblokir.
- Firmware OLED dirilis sebagai `0.3.0-oled` dengan nama manifest dan binary baru.
- Web server `serve.py` mengirim header `Cache-Control: no-store`.

## Validasi hardware dan MAVLink 6 Oktober 2026

Bagian ini menggantikan asumsi bench test lama dengan hasil yang sudah diamati
langsung pada dua LilyGO fisik:

- Air terdeteksi sebagai `COM13`; Ground terdeteksi sebagai `COM20`.
- Kedua MCU teridentifikasi sebagai ESP32-PICO-D4 revision 1.1.
- Register radio `RegVersion=0x12` terbaca stabil. Nilai ini membuktikan keluarga
  SX127x, tetapi tidak dapat membedakan SX1276 dan SX1278 sendirian.
- Sketch `RF_Sanity_Air` dan `RF_Sanity_Ground` sebelumnya berhasil melakukan
  PING/PONG dua arah pada 923 MHz dengan RSSI sekitar -60 dBm dan SNR positif.
- Default band mLRS yang semula 868 MHz sudah dikoreksi menjadi
  `SETUP_FREQUENCY_BAND_915_MHZ_FCC`, sesuai varian 915/923 MHz.
- Default USB serial Ground yang semula 115200 sudah dikoreksi menjadi 57600,
  sesuai Mission Planner dan seluruh tool TelemJaya.

### Diagnosis mLRS yang belum berhasil

Firmware produksi berbasis mLRS belum berhasil mengeluarkan heartbeat pada
Ground. Tes `diagnose_mavlink_path.ps1` memperlihatkan generator mengirim lebih
dari 1.300 frame tetapi `Ground RX` tetap 0 byte.

Membuka COM13 melalui CH9102 juga menyebabkan ESP32 Air reset. Gejalanya adalah
indikator Air yang semula connected kembali ke pola searching. Karena itu,
injeksi MAVLink dari terminal melalui COM13 tidak valid sebagai pengujian link
yang sedang berjalan. Mode heartbeat internal dibuat untuk menghindari reset,
tetapi firmware mLRS tetap belum meneruskan heartbeat sampai Ground.

Kesimpulan saat ini: kegagalan tersebut bukan berasal dari parameter ArduPilot,
karena kegagalan tetap terjadi tanpa flight controller. Area yang masih
bermasalah adalah integrasi state machine/binding/serial mLRS pada target baru.

### Proof 1: MAVLink langsung melalui USB Ground

Sketch `arduino/MAVLink_USB_Proof` mengirim MAVLink v1 langsung dari Ground ke
COM20 pada 57600. Verifier `verify_ground_mavlink_usb.ps1` mendapatkan:

```text
RX 9080 B | valid 266 | HB 33 | badCRC 0 | PASS
```

Ini membuktikan generator frame, checksum MAVLink, ESP32 UART0, CH9102, COM20,
dan baud 57600 bekerja dengan benar tanpa Mission Planner.

### Proof 2: MAVLink melalui LoRa 923 MHz

Untuk mengisolasi transport RF dari mLRS, dibuat dua sketch minimal:

- `arduino/MAVLink_RF_Proof_Air`: membuat HEARTBEAT MAVLink internal lalu
  mengirim frame mentah melalui LoRa 923 MHz.
- `arduino/MAVLink_RF_Proof_Ground`: menerima payload LoRa dan menulis byte
  asli ke COM20 pada 57600.

Keduanya memakai konfigurasi RF yang sebelumnya lulus PING/PONG: bandwidth
125 kHz, spreading factor 9, coding rate 4/7, sync word `0x34`, CRC radio aktif,
dan daya 10 dBm. Hasil terminal:

```text
RX 340 B | valid 20 | HB 20 | badCRC 0 | PASS
```

Dengan demikian jalur berikut sudah terbukti bekerja secara fisik:

```text
Air dummy MAVLink -> LoRa 923 MHz -> Ground -> USB COM20 -> parser MAVLink
```

Firmware proof sengaja tidak memuat OLED dan tidak memakai binding mLRS.
OLED kosong adalah perilaku yang diharapkan. Kedua sisi cocok melalui parameter
RF tetap; LED berubah setiap paket sebagai indikator aktivitas, bukan status
binding.

### Firmware yang sedang terpasang

Pada akhir sesi ini:

- Air menjalankan `MAVLink_RF_Proof_Air`.
- Ground menjalankan `MAVLink_RF_Proof_Ground`.
- Air mengirim satu HEARTBEAT valid per detik.
- Ground meneruskan setiap HEARTBEAT ke COM20.

Jangan menganggap firmware proof sebagai firmware penerbangan. Firmware ini
hanya baseline transport satu arah yang sudah terbukti.

### Langkah implementasi selanjutnya

1. Pertahankan proof 923 MHz sebagai baseline yang tidak diubah.
2. Buat firmware bridge produksi terpisah berdasarkan proof, bukan langsung
   menambah kompleksitas ke state machine mLRS yang belum lulus.
3. Ganti heartbeat internal Air dengan input UART FC pada GPIO3, 57600 baud.
4. Tambahkan packet framing berisi panjang, sequence, jenis paket, dan checksum
   agar byte stream MAVLink dapat dipotong dan dirakit kembali.
5. Tambahkan arah Ground ke Air untuk request parameter, mission, command, dan
   data GCS; link wajib half-duplex terjadwal agar tidak tabrakan.
6. Tambahkan timeout link, packet counter, loss, RSSI, SNR, dan byte rate.
7. Aktifkan kembali OLED setelah jalur dua arah stabil; status OLED harus
   berasal dari paket RF nyata, bukan hanya pola timer LED.
8. Uji terminal dua arah, kemudian Mission Planner, baru terakhir sambungkan
   Air ke UART7 Matek dan lakukan validasi parameter FC.

### Proof 3: bridge MAVLink half-duplex dua arah

Bridge terjadwal dibuat terpisah dari mLRS menggunakan framing TelemJaya,
sequence/acknowledgment, panjang payload, dan CRC16. Air menjadi pengatur slot:
downlink dikirim setiap 100 ms, kemudian Air menyediakan jendela 65 ms untuk
balasan Ground. Mode self-test membuat HEARTBEAT di Air dan mengembalikan PING
dari Ground melalui Air.

Hasil hardware pada COM20/57600:

```text
RX 426 B | valid 23 | HB 16 | PING return 7 | badCRC 0 | PASS-2WAY
```

Ini membuktikan kedua arah secara bersamaan:

```text
Air -> RF -> Ground -> USB                  (HEARTBEAT)
Ground USB -> RF -> Air -> RF -> Ground USB (PING echo)
```

Tidak ada CRC MAVLink yang gagal. Dengan demikian masalah transport radio dua
arah sudah selesai pada baseline bridge; integrasi FC dan tampilan tidak boleh
mengubah sketch proof lama.

### OLED dan pemisahan build setelah PASS-2WAY

OLED SSD1306 ditambahkan ke firmware bridge sebagai observasi 1 Hz. Layar
menampilkan role, `LINK OK`/`SEARCH`, frekuensi, jumlah paket TX/RX/BAD,
RSSI/SNR, serta total byte UART masuk/keluar. Update 1 Hz dipilih agar beban I2C
tidak mendominasi slot RF.

Terdapat dua alur upload yang tidak boleh tertukar:

- `upload_bridge_selftest.ps1`: Air membuat heartbeat dummy dan echo PING;
- `upload_bridge_production.ps1`: Air membaca/menulis MAVLink nyata lewat
  GPIO3/GPIO1 pada 57600 baud.

Setelah OLED ditambahkan, self-test wajib kembali menghasilkan `PASS-2WAY`
sebelum build produksi disambungkan ke flight controller.

### Proof 4: Mission Planner antar-laptop melalui dua LilyGO

Firmware bridge produksi kemudian dipasang pada kedua board. Untuk menguji
jalur seperti flight controller tanpa bergantung pada catu daya Matek, laptop
utama menjalankan `arduino/test_mavlink_dummy.ps1` pada COM Air. Script
`tools/mavlink_dummy_air.py` bertindak sebagai FC virtual dan mengirim frame
MAVLink valid pada 57600 baud, termasuk HEARTBEAT, SYS_STATUS, GPS_RAW_INT,
ATTITUDE, GLOBAL_POSITION_INT, dan VFR_HUD.

Ground dipindahkan ke laptop kedua dan dibuka langsung oleh Mission Planner
pada 57600 baud. Mission Planner berhasil connected dan menampilkan attitude,
GPS 3D fix, posisi peta, altitude, baterai, serta data HUD secara real-time.

Jalur yang sudah terbukti adalah:

```text
Laptop utama / FC dummy
  -> USB-UART LilyGO Air
  -> LoRa 923 MHz
  -> LilyGO Ground
  -> USB laptop kedua
  -> Mission Planner
```

Pengujian ini sepenuhnya melewati dua radio LilyGO. Tidak digunakan TCP, UDP,
Wi-Fi, forwarding jaringan, atau koneksi langsung antarlaptop. Dengan demikian
keberhasilan Mission Planner merupakan bukti nyata bahwa byte MAVLink melewati
bridge RF TelemJaya.

### Batas validasi terakhir

Integrasi final Matek H743-WLITE ke UART LilyGO Air belum diuji. Karena itu
project belum boleh disebut tervalidasi end-to-end menggunakan flight
controller nyata. Rencana wiring untuk pengujian selanjutnya:

```text
Matek TX7  -> LilyGO RXD / GPIO3
Matek RX7  <- LilyGO TXD / GPIO1
Matek GND  <-> LilyGO GND
Matek 5V   tidak disambungkan saat Air ditenagai charger USB
```

Untuk bench test, LilyGO Air dapat ditenagai charger USB 5 V dan Matek ditenagai
secara terpisah. Common ground tetap wajib. Antena harus terpasang sebelum radio
dinyalakan. Mission Planner hanya membuka USB Ground; COM Air tidak boleh dibuka
oleh dummy script atau Serial Monitor saat FC nyata digunakan.
