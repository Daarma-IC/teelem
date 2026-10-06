# Status Pengerjaan TelemJaya

Dokumen ini merangkum pekerjaan yang sudah dilakukan sampai 30 September 2026,
hasil yang sudah diverifikasi, dan pekerjaan yang masih harus diuji pada hardware.

## Tujuan project

Membuat link telemetri MAVLink dua arah menggunakan dua LilyGO T3 V1.6.1
ESP32-PICO-D4 + SX1276:

```text
ArduPilot TELEM <-> LilyGO Air <-> mLRS RF <-> LilyGO Ground <-> USB <-> Mission Planner
```

Air dan Ground tidak membaca setiap sensor secara langsung. ArduPilot mengolah
sensor dan mengirim message MAVLink melalui port TELEM. TelemJaya bertugas
mengangkut byte stream tersebut dua arah.

## Repository yang dijadikan basis

- `../mLRS`: radio link, FHSS, ARQ, binding, MAVLink, dan MAVLinkX.
- `../ardupilot`: referensi serial protocol dan endpoint MAVLink sensor.
- `../mLRS-hardware`: referensi hardware mLRS dan konvensi target board.

Seluruh perubahan TelemJaya ditempatkan di folder baru `telemjaya project/`;
ketiga repository sumber tidak dijadikan tempat implementasi utama.

## Pekerjaan yang sudah dilakukan

### 1. Analisis hardware

- Target ditetapkan sebagai LilyGO T3 V1.6.1 non-TCXO dengan SX1276 900 MHz.
- Mapping SPI/radio ditetapkan: SCK 5, MISO 19, MOSI 27, CS 18, RESET 23,
  DIO0 26, DIO1 33, dan DIO2 32.
- LED status firmware memakai GPIO25.
- Tombol BOOT GPIO0 dipakai sebagai permintaan bind setelah startup normal.

### 2. Penambahan target firmware mLRS

Target PlatformIO baru dibuat:

```text
rx-lilygo-t3-v161-900  = Air/vehicle
tx-lilygo-t3-v161-900  = Ground/GCS
```

HAL baru berada di:

```text
firmware/mLRS/Common/hal/esp/rx-hal-lilygo-t3-v161-900-esp32.h
firmware/mLRS/Common/hal/esp/tx-hal-lilygo-t3-v161-900-esp32.h
```

Target tersebut sudah diregistrasikan pada `esp-device_conf.h`, `esp-hal.h`,
dan `firmware/platformio.ini`.

### 3. Jalur serial

- Air memakai UART0: GPIO3 sebagai RX dan GPIO1 sebagai TX menuju TELEM FC.
- Ground memakai UART0 GPIO3/RX dan GPIO1/TX menuju CH9102 USB onboard.
- Baud integrasi awal adalah 57600.
- Konfigurasi awal ArduPilot: `SERIALx_PROTOCOL=2` dan `SERIALx_BAUD=57`.

### 4. Dependency dan MAVLink

- Submodule `sx12xx-lib`, `stm32ll-lib`, dan `fastmavlink` disiapkan pada fork.
- Header MAVLink yang dibutuhkan firmware sudah digenerate dan dimasukkan ke
  workspace firmware.
- Peta sensor, message MAVLink, dan arah kendali sudah dicatat pada
  `MAVLINK_ENDPOINTS.md`.

### 5. Sistem status LED dan binding

Firmware versi `0.2.0-bind-led` memiliki perilaku berikut:

| Pola LED hijau | Arti |
|---|---|
| Satu kedip panjang, kemudian jeda | Mencari pasangan tersimpan |
| Dua kedip cepat, kemudian jeda | Bind mode / ready binding |
| Menyala terus | Air dan Ground sudah connected |

Urutan otomatis:

1. Setelah boot, unit mencari pasangan tersimpan selama maksimal 20 detik.
2. Jika belum connected, unit otomatis masuk bind mode.
3. Air dan Ground yang berada dalam bind mode menukar dan menyimpan parameter link.
4. Setelah link connected, LED hijau menyala terus.

Bind manual dapat diminta dengan menahan tombol BOOT sekitar empat detik setelah
board selesai boot. BOOT tidak boleh ditahan saat power-on/reset karena GPIO0
juga memilih ROM bootloader ESP32.

### 6. Web flasher

Web flasher berbasis HTML, CSS, JavaScript, dan ESP Web Tools telah dibuat di
`web-flasher/`. Tersedia manifest terpisah untuk Air dan Ground beserta binary
bootloader, partition table, boot application, dan application image.

Menjalankan web flasher:

```powershell
cd "D:\aaa\asambly\telemjaya project\web-flasher"
python serve.py
```

Kemudian buka `http://localhost:8000` memakai Chrome atau Edge.
Server khusus ini menonaktifkan browser cache. Manifest dan application binary
juga menggunakan nama berversi agar build lama tidak dipakai kembali.

### 7. OLED telemetry onboard

Firmware `0.3.0-oled` mengaktifkan SSD1306 128x64 onboard melalui I2C GPIO21
(SDA) dan GPIO22 (SCL) pada Air dan Ground. OLED diperbarui sekitar dua kali
per detik dan transfer I2C dijalankan pada task Core 0 agar loop radio Core 1
tidak diblokir.

Spesifikasi layar T3 V1.6.1 adalah OLED SSD1306 0,96 inci, resolusi 128x64,
alamat I2C `0x3C`. Font status memakai sel 6x8 piksel; teks dibatasi maksimal
21 karakter per baris. Unit Ground yang diuji memiliki orientasi panel
horizontal-mirror, sehingga framebuffer Ground dikompensasi di software.

Informasi yang ditampilkan:

- role perangkat: `TelemJaya AIR` atau `TelemJaya GROUND`;
- state `SEARCH`, `BINDING`, atau `CONNECTED`;
- countdown menuju auto-bind ketika searching;
- mode RF dan frequency band;
- Link Quality, RSSI, dan MAVLink packet quality;
- jalur UART FC atau USB dan baud 57600.

### 8. Build firmware terakhir

Kedua target berhasil compile dan link dengan status `SUCCESS`:

| Target | RAM | Flash program | App binary |
|---|---:|---:|---:|
| Air/RX OLED | 31,568 byte | 364,629 byte | 364,992 byte |
| Ground/TX OLED | 32,344 byte | 376,593 byte | 376,960 byte |

Artefak terakhir:

| File | SHA-256 |
|---|---|
| `telemjaya-air-t3-v161-sx1276.bin` | `82BA484E681584F3D790BA59C33DADEC1CF3451997CEC6DCA9F9C05CDB73A979` |
| `telemjaya-ground-t3-v161-sx1276.bin` | `45CE9EBD68ACCB6553754304D6F4D632C224EBFFCAA205D132867AA3C61246CE` |

Binary dalam `artifacts/` sudah dicocokkan dengan binary yang dipakai oleh
`web-flasher/firmware/`; checksum keduanya sama.

## Yang sudah diverifikasi

- Source Air dan Ground berhasil compile serta link.
- Image ESP32 Air dan Ground berhasil dibuat.
- Manifest web flasher valid dan menunjuk offset flash yang benar.
- Binary artifacts identik dengan binary pada web flasher.
- Logic LED, timer pencarian 20 detik, auto-bind, dan BOOT bind masuk ke build.

## Yang belum diverifikasi di hardware

- Kedua unit benar-benar menyelesaikan binding dan LED berubah solid.
- Bind phrase/parameter tetap tersimpan setelah power cycle.
- SX1276 menerima dan mengirim paket stabil pada kedua board.
- UART Air menerima heartbeat dari ArduPilot.
- Mission Planner membuka COM Ground dan menyelesaikan parameter download.
- Mission upload/download dan command dua arah mendapat respons.
- Stabilitas link selama minimal 30 menit.
- Range, packet loss, RSSI/LQ, failsafe, dan RF power.
- Kesesuaian frequency plan dan daya pancar terhadap wilayah operasi.
- Varian board benar-benar non-TCXO; varian TCXO membutuhkan HAL berbeda.

## Tahap pengujian berikutnya

1. Flash ulang kedua board dengan firmware `0.2.0-bind-led`.
2. Nyalakan dengan antena terpasang dan jarak awal sekitar 1-2 meter.
3. Amati transisi satu kedip lambat, dua kedip cepat, lalu solid.
4. Power-cycle kedua unit dan pastikan reconnect terjadi sebelum 20 detik.
5. Sambungkan Air ke port TELEM ArduPilot menggunakan silang TX/RX dan common GND.
6. Buka COM Ground di Mission Planner pada 57600 baud.
7. Uji heartbeat, parameter, mission, command, dan monitoring sensor.
8. Lakukan soak test dan range test sebelum penggunaan terbang.

### 9. Perbaikan binding 0.3.6

- Foto pengujian memperlihatkan Ground sudah `BINDING`, tetapi Air berhenti di
  boot stage `RX CLOCK OK`; artinya loop radio Air belum berjalan.
- Inisialisasi UART Air yang sebelumnya melakukan `end()/begin()` dua kali
  disederhanakan menjadi satu kali melalui `Serials.Init()`.
- Checkpoint `UART START`, `UART OK`, dan `SERIAL OK` ditambahkan ke OLED Air.
- BOOT pada kedua role kini cukup ditekan singkat setelah debounce untuk masuk
  binding.
- Layar binding menampilkan counter paket `TX`, `RX`, dan `BAD` supaya jalur RF
  dapat didiagnosis tanpa menebak.
- Air dan Ground versi `0.3.6-bindfix` berhasil compile dan link.

### 10. Build Arduino IDE

- Ditambahkan sketch terpisah `arduino/TelemJaya_Air` dan
  `arduino/TelemJaya_Ground` untuk upload langsung tanpa web flasher.
- Wrapper mempertahankan source mLRS sebagai translation unit terpisah.
- HAL timer mendukung API Arduino-ESP32 2.x dan 3.x.
- Kedua sketch berhasil dibangun memakai Arduino CLI 1.5.1, ESP32 core 3.3.10,
  dan target `esp32:esp32:pico32`.

### 11. Firmware 0.4.0 polling IRQ dan console

- Tes LoRa ping/pong membuktikan komunikasi bidirectional 923 MHz dengan RSSI
  sekitar -60 dBm dan SNR positif pada kedua LilyGO.
- Target LilyGO tidak lagi melakukan transaksi SPI SX1276 dari GPIO ISR.
- IRQ RX_DONE/TX_DONE/TIMEOUT dipoll setiap 250 us dari main loop dan diteruskan
  ke state machine mLRS yang sama.
- Air menyediakan console USB 115200 berisi state link, counter bind, LQ, RSSI,
  MAVLink LQ, serta throughput FC-ke-Ground dan Ground-ke-FC.
- Script production clean upload ditambahkan untuk Air dan Ground.
- Kedua firmware berhasil compile dengan Arduino-ESP32 3.3.10.

## Batas status saat ini

Project sudah berada pada tahap firmware buildable dan siap bench test.

`SUCCESS` dari compiler tidak sama dengan validasi RF atau flight-tested. Binary
belum boleh disebut stable sebelum seluruh pengujian hardware di atas selesai.

## Status terbaru 6 Oktober 2026

Status lama yang menyebut RF belum diverifikasi sudah tidak berlaku.
Pengujian hardware kini memberikan hasil berikut:

| Lapisan | Hasil | Bukti |
|---|---|---|
| SX127x SPI | Lulus | `RegVersion=0x12` stabil |
| Raw RF 923 MHz | Lulus | PING/PONG dua arah, RSSI sekitar -60 dBm |
| MAVLink langsung Ground USB | Lulus | 266 frame valid, 33 heartbeat, 0 CRC error |
| MAVLink Air -> LoRa -> Ground -> USB | Lulus | 20 heartbeat valid, 0 CRC error |
| Bridge MAVLink RF dua arah | Lulus | 16 HB, 7 PING return, 0 CRC error, `PASS-2WAY` |
| FC dummy -> Air -> RF -> Ground -> Mission Planner | Lulus | Mission Planner connected dan menampilkan HUD/GPS/attitude |
| Integrasi mLRS TelemJaya | Belum lulus | Ground menghasilkan 0 byte pada diagnosis |
| UART flight controller | Belum diuji valid | Menunggu bridge produksi stabil |
| Mission Planner dengan FC dummy | Lulus | Dua laptop dan dua LilyGO; tanpa TCP/UDP/Wi-Fi |
| Mission Planner dengan Matek nyata | Belum diuji | Wiring dan catu daya final belum diuji |

Baseline yang terbukti adalah sketch `MAVLink_RF_Proof_Air` dan
`MAVLink_RF_Proof_Ground`. Keduanya menggunakan LoRa 923 MHz dengan parameter
tetap dan tidak menggunakan binding. OLED sengaja tidak aktif pada build proof.

Temuan konfigurasi penting:

- default RF lama 868 MHz salah untuk hardware 915/923 MHz dan sudah diubah ke
  band 915 MHz FCC;
- default serial Ground lama 115200 tidak cocok dengan tool 57600 dan sudah
  diubah menjadi 57600;
- membuka COM13 melalui CH9102 me-reset Air, sehingga COM13 tidak boleh dipakai
  sebagai injector sambil mempertahankan koneksi RF.

Bridge MAVLink transparan dua arah berbasis raw-LoRa sekarang sudah lulus pada
hardware. OLED/status sudah ditambahkan ke source bridge dan kedua role lolos
compile (Air self-test 328.736 byte; Ground 328.416 byte). Prioritas berikutnya
telah tercapai sebagian: build produksi berhasil membawa payload FC dummy dari
laptop utama melalui kedua LilyGO sampai Mission Planner di laptop kedua.
Pengujian tersebut tidak memakai transport jaringan apa pun.

Status final yang belum dilakukan adalah menghubungkan UART7 Matek H743-WLITE
ke UART Air dan memverifikasi MAVLink dari flight controller nyata. Sampai tes
itu dilakukan, hasil FC dummy tidak boleh dicatat sebagai bukti bahwa wiring,
parameter serial, dan catu daya Matek sudah benar.

## Firmware production yang tervalidasi dengan simulator

Ini adalah pasangan firmware yang dipakai ketika Mission Planner berhasil
connected melalui dua LilyGO pada 6 Oktober 2026:

| Role | Source | Binary hasil build | SHA-256 |
|---|---|---|---|
| Air production | `arduino/TelemJaya_Bridge_Air/TelemJaya_Bridge_Air.ino` | `.build/TelemJaya_Bridge_Air_Production/TelemJaya_Bridge_Air.ino.bin` | `B89F72D4F19D85BE73DC1355A1A02A7B570DAEE950E0087AA090A4DE9AF0E72D` |
| Ground production | `arduino/TelemJaya_Bridge_Ground/TelemJaya_Bridge_Ground.ino` | `.build/TelemJaya_Bridge_Ground_Production/TelemJaya_Bridge_Ground.ino.bin` | `B40D0BF9F548FE0C951DBD78BD06490653B46027267BDDC5AC1A9951F9F3C8D7` |

Keduanya di-compile dan di-upload memakai:

```powershell
.\upload_bridge_production.ps1 -AirPort COM13 -GroundPort COM20
```

Setelah firmware production terpasang, pengujian yang berhasil memakai:

```powershell
.\test_mavlink_dummy.ps1 -AirPort COM13 -Baud 57600
```

Ground dipasang ke laptop kedua dan Mission Planner membuka COM Ground pada
57600 baud. Mission Planner berhasil menerima telemetri. Ini merupakan status
`VALIDATED WITH LAPTOP FC SIMULATOR`, bukan validasi Matek nyata.

Jangan mengganti firmware tersebut dengan `TelemJaya_Bridge_Air_Test` untuk
penggunaan FC. Build bertanda `SELFTEST` membuat heartbeat internal dan tidak
membaca input UART FC seperti build production.
