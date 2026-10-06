# Web Flasher

Folder ini berisi web flasher lokal berbasis ESP Web Tools untuk firmware Air
dan Ground. Firmware masih berstatus development dan belum flight-tested.

Layout kandidat yang telah dihasilkan oleh PlatformIO:

| Offset | Artifact |
|---:|---|
| `0x1000` | `../artifacts/esp32-pico-bootloader.bin` |
| `0x8000` | `../artifacts/esp32-pico-partitions.bin` |
| `0xE000` | `../artifacts/esp32-boot-app0.bin` |
| `0x10000` | firmware Air atau Ground |

Manifest Air dan Ground sudah aktif untuk pengujian lokal.

## Status LED setelah flashing

- Satu kedip panjang: mencari pasangan tersimpan, maksimal 20 detik.
- Dua kedip cepat: siap binding.
- Hijau menyala terus: Air dan Ground sudah connected.

Untuk langsung masuk bind, biarkan board selesai boot lalu tahan tombol BOOT
sekitar 4 detik. Jangan menahan BOOT saat power-on atau reset.
s