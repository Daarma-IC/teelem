# TelemJaya Project

Build langsung melalui Arduino IDE tersedia di
[`arduino/README.md`](arduino/README.md).

TelemJaya adalah link telemetri MAVLink dua arah untuk flight controller
ArduPilot menggunakan sepasang LilyGO T3 V1.6.1 (ESP32-PICO-D4 + SX1276).

```text
ArduPilot TELEM <-> LilyGO Air <-> mLRS RF <-> LilyGO Ground <-> USB <-> Mission Planner
```

## Struktur

- `firmware/` - fork kerja mLRS dengan target LilyGO Air dan Ground.
- `docs/BRAINSTORMING.md` - keputusan arsitektur dan batasan desain.
- `docs/WALKTHROUGH.md` - perubahan, cara build, dan status pengujian.
- `docs/MAVLINK_ENDPOINTS.md` - peta data sensor ArduPilot sampai GCS.
- `docs/PROJECT_STATUS.md` - ringkasan seluruh pekerjaan, hasil, dan sisa pengujian.
- `artifacts/` - tujuan binary firmware yang sudah tervalidasi.
- `web-flasher/` - tempat implementasi web flasher pada tahap berikutnya.

## Upstream yang menjadi basis

Project ini memanfaatkan tiga repository lokal tanpa mengubahnya:

- `../mLRS` sebagai transport radio, framing, ARQ, FHSS, dan MAVLinkX.
- `../ardupilot` sebagai referensi endpoint serial dan producer MAVLink.
- `../mLRS-hardware` sebagai referensi konvensi hardware dan desain perangkat mLRS.

Lihat `docs/WALKTHROUGH.md` untuk commit upstream yang dipakai saat project dibuat.

## Binding dan indikator hijau

Saat dinyalakan, Air dan Ground mencari pasangan yang pernah tersimpan selama
20 detik. Jika tidak ditemukan, keduanya otomatis masuk bind mode.

| LED hijau GPIO25 | Status |
|---|---|
| Satu kedip panjang, lalu jeda | Mencari pasangan tersimpan |
| Dua kedip cepat, lalu jeda | Bind mode / siap binding |
| Menyala terus | RF link benar-benar connected |

Untuk langsung masuk bind, biarkan board boot normal lalu tahan tombol **BOOT**
sekitar 4 detik. Jangan menahan BOOT saat power-on atau reset karena GPIO0 juga
memilih ROM bootloader ESP32.

## Build indikator/binding 0.2.0

| Target | App binary | SHA-256 |
|---|---:|---|
| Air/RX | 345,952 byte | `7DCE2BA1C208DD31E9C62387908CC7B2E2A42D6EAD87BF063AC1331E99767B7F` |
| Ground/TX | 358,224 byte | `9AB19E8FDF6DCA7915B2ECCC8D42BA3C0D5CB06D4FA8318B0443F5476121E5E3` |

Board : 
https://lilygo.cc/en-us/products/lora3?srsltid=AU7gw4VAilKgcVyQueNiLQqWYmaD8xBVq7kshdBRT2OmsN7PKlrY5hYJ



cd "D:\aaa\asambly\telemjaya project\arduino"
.\test_mavlink_dummy.ps1 -AirPort COM13 -Baud 57600