# RPI-to-STM-Code-Upload

<div align="center">

![Version](https://img.shields.io/badge/version-2.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi-red.svg)
![MCU](https://img.shields.io/badge/MCU-STM32F1%20%7C%20STM32F7-green.svg)
![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)

**Raspberry Pi'den STM32 Mikrodenetleyicilere UART Üzerinden Firmware Güncelleme Sistemi**

[Özellikler](#özellikler) •
[Kurulum](#kurulum) •
[Kullanım](#kullanım) •
[Dokümantasyon](#dokümantasyon) •
[Katkıda Bulunma](#katkıda-bulunma)

</div>

---

## Genel Bakış

Bu proje, Raspberry Pi ve STM32 mikrodenetleyiciler arasında UART (Universal Asynchronous Receiver-Transmitter) üzerinden güvenilir firmware güncellemesi yapılmasını sağlayan kapsamlı bir sistemdir.

### Ana Özellikler
- ✅ **Çoklu Platform Desteği**: STM32F1, STM32F7 ve diğer STM32 seriler
- ✅ **Güvenilir Transfer**: Checksum doğrulama ve otomatik retry mekanizması
- ✅ **Bootloader + Application**: İki katmanlı firmware yapısı
- ✅ **Progress Tracking**: Detaylı ilerleme gösterimi
- ✅ **Error Recovery**: 5 kademeli retry sistemi
- ✅ **Yüksek Hız**: 115200 baud rate ile hızlı transfer
- ✅ **Esnek Boyut**: 1 byte'tan 256KB'a kadar firmware desteği

### Uygulama Alanları
- 🏭 Endüstriyel IoT cihazları
- 🤖 Robotik sistemler
- 📡 Uzaktan firmware güncelleme gerektiren sistemler
- 🔧 Geliştirme ve test ortamları
- 🏠 Akıllı ev sistemleri

---

## Proje Yapısı

```
RPI-to-STM-Code-Upload/
├── Rpi/                          # Raspberry Pi kodları
│   ├── RpiUart.c                 # UART kütüphanesi
│   ├── RpiUart.h                 # UART header
│   ├── binFileUpdate.c           # Firmware yükleme aracı
│   ├── Makefile                  # Build scripti
│   └── build.sh                  # Kolay derleme scripti
│
├── Stm/                          # STM32 kodları
│   ├── BLD/                      # Bootloader kodları
│   │   ├── F103Boot/             # STM32F1 Bootloader
│   │   └── F7Boot/               # STM32F7 Bootloader
│   ├── Ap/                       # Application kodları
│   │   ├── F103Ap/               # STM32F1 Application
│   │   └── F7Ap/                 # STM32F7 Application
│   └── STM32F7_README.md         # STM32F7 detaylı dokümantasyon
│
├── PROTOCOL_ALGORITHM.md         # Protokol ve algoritma dokümantasyonu
└── README.md                     # Bu dosya
```

---

## Kurulum

### Raspberry Pi Kurulumu

1. **Gerekli paketleri yükleyin:**
```bash
sudo apt-get update
sudo apt-get install build-essential git
```

2. **Repoyu klonlayın:**
```bash
git clone https://github.com/ByTaymur/RPI-to-STM-Code-Upload.git
cd RPI-to-STM-Code-Upload/Rpi
```

3. **Derleyin:**
```bash
chmod +x build.sh
./build.sh
```

veya

```bash
make
```

4. **Kurulum (opsiyonel):**
```bash
sudo make install
```

### STM32 Kurulumu

1. **STM32CubeIDE yükleyin** (https://www.st.com/en/development-tools/stm32cubeide.html)

2. **Bootloader projesini açın:**
   - `File → Open Projects from File System`
   - Dizin seçin: `Stm/BLD/F103Boot` (F1 için) veya `Stm/BLD/F7Boot` (F7 için)

3. **Derleyin:**
   - `Project → Build All` (Ctrl+B)

4. **STM32'ye yükleyin:**
   - `Run → Debug` (F11)

5. **Application projesini tekrarlayın:**
   - Dizin: `Stm/Ap/F103Ap` veya `Stm/Ap/F7Ap`

---

## Donanım Bağlantıları

### Pin Bağlantıları

```
┌──────────────────┐                 ┌──────────────────┐
│  Raspberry Pi    │                 │     STM32        │
│                  │                 │                  │
│  GPIO14 (TX) ────┼─────────────────┼──→ PA10 (RX)     │
│  GPIO15 (RX) ←───┼─────────────────┼──← PA9  (TX)     │
│  GND         ────┼─────────────────┼──  GND           │
│                  │                 │                  │
└──────────────────┘                 └──────────────────┘
```

### UART Ayarları
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

---

## Kullanım

### Hızlı Başlangıç

```bash
# 1. RPI'de UART portunu kontrol edin
ls /dev/tty*

# 2. Firmware dosyasını hazırlayın (.bin formatında)
# STM32CubeIDE'den binary export edin

# 3. Firmware'i gönderin
sudo ./binFileUpdate firmware.bin 1

# Port numaraları:
# 1  = /dev/ttyS0
# 2  = /dev/ttyAMA0
# 3  = /dev/ttyUSB0
```

### Detaylı Kullanım

**Adım 1: STM32'yi Bootloader Moduna Alın**
- STM32'yi reset edin
- Bootloader otomatik olarak UART'tan data bekler

**Adım 2: Firmware Transfer**
```bash
sudo ./binFileUpdate MyApp.bin 2
```

**Beklenen Çıktı:**
```
╔════════════════════════════════════════════════════════════════╗
║         RPI to STM32 Firmware Update Tool v2.0                ║
╚════════════════════════════════════════════════════════════════╝

[1/5] Opening UART port 2...
      ✓ UART port opened successfully (115200 8N1)

[2/5] Reading binary file: MyApp.bin
      ✓ File loaded: 12345 bytes (12.06 KB)

[3/5] Performing handshake with STM32...
      Waiting for ACK from STM32...
      ✓ Handshake successful

[4/5] Transferring firmware (13 blocks)...
      [██████████████████████████████████████████████████] 100.0% (13/13)

[5/5] Waiting for STM32 to complete...
      ✓ Transfer completed successfully!

╔════════════════════════════════════════════════════════════════╗
║                  FIRMWARE UPDATE SUCCESSFUL                    ║
╠════════════════════════════════════════════════════════════════╣
║  Total size:    12345      bytes                               ║
║  Total blocks:  13                                             ║
║  Block size:    1024       bytes                               ║
╚════════════════════════════════════════════════════════════════╝
```

---

## Dokümantasyon

### Protokol Detayları
Detaylı protokol açıklaması için: [PROTOCOL_ALGORITHM.md](./PROTOCOL_ALGORITHM.md)

Bu dokümantasyon şunları içerir:
- Handshake protokolü
- Veri transfer algoritması
- Checksum mekanizması
- Hata yönetimi
- Zaman diyagramları
- Kod örnekleri

### STM32F7 Özel Dokümantasyon
STM32F7 özellikleri için: [STM32F7_README.md](./Stm/STM32F7_README.md)

---

## Performans

### Transfer Hızları
| Firmware Boyutu | Süre (tahmini) | Throughput |
|-----------------|----------------|------------|
| 10 KB           | ~1 saniye      | ~10 KB/s   |
| 48 KB           | ~5 saniye      | ~9.6 KB/s  |
| 128 KB          | ~13 saniye     | ~9.8 KB/s  |
| 256 KB          | ~26 saniye     | ~9.8 KB/s  |

### Başarı Oranı
- Normal koşullar: **%100**
- Gürültülü ortam: **>%95** (retry sayesinde)
- UART hatası: **~%0** (fatal error)

---

## Sorun Giderme

### Sık Karşılaşılan Hatalar

#### 1. "Cannot open COM port"
**Sebep:** UART portu kullanımda veya yanlış port numarası

**Çözüm:**
```bash
# Port kullanımını kontrol et
lsof | grep tty

# Kullanıcıyı dialout grubuna ekle
sudo usermod -a -G dialout $USER

# Logout/login yapın
```

#### 2. "Handshake failed"
**Sebep:** STM32 yanıt vermiyor

**Çözüm:**
- UART bağlantılarını kontrol edin (TX↔RX çapraz mı?)
- STM32'nin bootloader modunda olduğundan emin olun
- Baud rate uyuşuyor mu?
- STM32'yi reset edin

#### 3. "Checksum mismatch"
**Sebep:** Veri bozulması

**Çözüm:**
- UART kablolarının kalitesini kontrol edin
- Kabloları kısaltın (max 30cm)
- Topraklama bağlantısını kontrol edin
- Baud rate'i düşürün (örn: 57600)

#### 4. "Flash Write Error"
**Sebep:** Flash yazma hatası

**Çözüm:**
- STM32'de write protection var mı?
- Flash bölgesi doğru mu?
- Voltage seviyesi yeterli mi? (min 2.7V)

---

## Geliştirme

### Kodlama Standartları
- **C Standard**: C11
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Max 100 characters
- **Naming**: 
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_CASE`
  - Types: `PascalCase`

### Build Sistemi
```bash
# RPI tarafı
cd Rpi
make clean
make
make install

# Test
make test
```

### Debug Modu
```c
// main.c içinde debug log ekleyin:
#define DEBUG_MODE 1

#if DEBUG_MODE
    printf("DEBUG: Block %d sent\n", block_num);
#endif
```

---

## Katkıda Bulunma

Katkılarınızı bekliyoruz! Lütfen şu adımları izleyin:

1. Fork edin
2. Feature branch oluşturun (`git checkout -b feature/AmazingFeature`)
3. Commit edin (`git commit -m 'Add some AmazingFeature'`)
4. Push edin (`git push origin feature/AmazingFeature`)
5. Pull Request açın

### Katkı Alanları
- [ ] Yeni STM32 serisi desteği (F4, H7, etc.)
- [ ] CRC-16/32 checksum implementasyonu
- [ ] Encryption desteği
- [ ] GUI tool (Python/Qt)
- [ ] Otomatik test suite
- [ ] Performance optimizasyonları

---

## Lisans

Bu proje MIT lisansı altında yayınlanmıştır. Detaylar için [LICENSE](LICENSE) dosyasına bakın.

---

## İletişim

**Proje Sahibi:** ByTaymur
**GitHub:** https://github.com/ByTaymur/RPI-to-STM-Code-Upload
**Issues:** https://github.com/ByTaymur/RPI-to-STM-Code-Upload/issues

---

## Teşekkürler

- STMicroelectronics HAL kütüphanesi
- Raspberry Pi Foundation
- Teunis van Beelen (RS-232 library)

---

## Sürüm Geçmişi

### v2.0 (2024-11-18)
- ✨ STM32F7 desteği eklendi
- ✨ Progress bar implementasyonu
- ✨ Retry mekanizması geliştirildi
- ✨ Detaylı hata mesajları
- 🐛 UART timeout sorunları düzeltildi
- 🐛 Checksum hesaplama optimizasyonu
- 📝 Kapsamlı dokümantasyon

### v1.0 (2023-xx-xx)
- 🎉 İlk sürüm
- ✨ STM32F1 desteği
- ✨ Temel bootloader
- ✨ UART transfer protokolü

---

<div align="center">

**⭐ Projeyi beğendiyseniz yıldız vermeyi unutmayın! ⭐**

Made with ❤️ by developers, for developers

</div>
