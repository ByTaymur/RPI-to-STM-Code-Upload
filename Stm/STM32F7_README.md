# STM32F7 Bootloader ve Application Kılavuzu

## Genel Bakış

Bu proje, STM32F7 mikrodenetleyici için UART üzerinden Raspberry Pi ile iletişim kurabilen ve firmware güncellemesi yapabilen bir bootloader ve application içermektedir.

## Özellikler

### STM32F7 Avantajları
- **Yüksek Performans**: 216 MHz CPU hızı
- **Cache Desteği**: I-Cache ve D-Cache aktif
- **Gelişmiş Flash**: Sektör bazlı silme (32KB-256KB sektörler)
- **Geniş Bellek**: 1MB Flash, 320KB RAM

## Bellek Haritası

### Flash Bellek Yapısı
```
┌─────────────────────────────────────────┐
│ 0x08000000 - 0x08007FFF (32KB)          │
│ SECTOR 0: Bootloader                    │
├─────────────────────────────────────────┤
│ 0x08008000 - 0x080FFFFF (992KB)         │
│ SECTOR 1-7: Application                 │
└─────────────────────────────────────────┘
```

### Flash Sektör Detayları
- **Sector 0**: 0x08000000 - 0x08007FFF (32 KB) - Bootloader
- **Sector 1**: 0x08008000 - 0x0800FFFF (32 KB) - Application
- **Sector 2**: 0x08010000 - 0x08017FFF (32 KB)
- **Sector 3**: 0x08018000 - 0x0801FFFF (32 KB)
- **Sector 4**: 0x08020000 - 0x0803FFFF (128 KB)
- **Sector 5**: 0x08040000 - 0x0807FFFF (256 KB)
- **Sector 6**: 0x08080000 - 0x080BFFFF (256 KB)
- **Sector 7**: 0x080C0000 - 0x080FFFFF (256 KB)

## Proje Yapısı

```
Stm/
├── BLD/
│   └── F7Boot/
│       ├── Core/
│       │   ├── Inc/
│       │   │   └── main.h
│       │   └── Src/
│       │       └── main.c
│       └── STM32F7_Bootloader.ld
└── Ap/
    └── F7Ap/
        ├── Core/
        │   ├── Inc/
        │   │   └── main.h
        │   └── Src/
        │       └── main.c
        └── STM32F7_Application.ld
```

## Bootloader Özellikleri

### Ana Özellikler
- UART üzerinden firmware alımı
- Flash sektör silme ve yazma
- Application doğrulama
- Güvenli application başlatma

### Clock Konfigürasyonu
- HSE: 8 MHz
- PLL: 432 MHz (VCO)
- SYSCLK: 216 MHz
- AHB: 216 MHz
- APB1: 54 MHz
- APB2: 108 MHz
- USB: 48 MHz

### UART Ayarları
- **USART1**: Raspberry Pi iletişimi (TX/RX)
  - Baud Rate: 115200
  - Data Bits: 8
  - Stop Bits: 1
  - Parity: None

- **USART2**: Debug/Log çıkışı (TX only)
  - Baud Rate: 115200

### Flash Yazma Protokolü
1. Raspberry Pi'den data header alınır: `{LENGTH}`
2. Bootloader 'O' karakteri ile onay gönderir
3. 1024 byte bloklar halinde data alınır
4. Her blok için checksum gönderilir
5. Tüm data alındıktan sonra application'a geçiş yapılır

## Application Özellikleri

### Ana Özellikler
- 0x08008000 adresinden çalışır
- Bootloader tarafından başlatılır
- UART komut işleme
- LED kontrolü
- Sistem durumu raporlama

### Desteklenen Komutlar (UART1)
- `L` veya `l`: LED toggle
- `V` veya `v`: Versiyon bilgisi
- `S` veya `s`: Sistem durumu

### Çalışma Akışı
1. Cache'ler etkinleştirilir (I-Cache, D-Cache)
2. HAL kütüphanesi başlatılır
3. System clock 216 MHz'e ayarlanır
4. Periferaller (GPIO, UART) başlatılır
5. Ana döngü:
   - Her 500ms LED toggle
   - UART komutları dinlenir
   - Durum bilgisi yazdırılır

## Derleme ve Yükleme

### Gereksinimler
- STM32CubeIDE veya ARM GCC toolchain
- STM32F7xx HAL kütüphaneleri
- ST-Link programlayıcı

### Bootloader Derleme
```bash
cd Stm/BLD/F7Boot
# STM32CubeIDE ile aç ve derle
# veya
arm-none-eabi-gcc ... -T STM32F7_Bootloader.ld
```

### Application Derleme
```bash
cd Stm/Ap/F7Ap
# STM32CubeIDE ile aç ve derle
# veya
arm-none-eabi-gcc ... -T STM32F7_Application.ld
```

### Binary Oluşturma
```bash
# Bootloader binary
arm-none-eabi-objcopy -O binary F7Boot.elf F7Boot.bin

# Application binary
arm-none-eabi-objcopy -O binary F7Ap.elf F7Ap.bin
```

## Raspberry Pi ile Kullanım

### Bağlantı Şeması
```
STM32F7          Raspberry Pi
USART1_TX (PA9)  ←→  RX (GPIO15)
USART1_RX (PA10) ←→  TX (GPIO14)
GND              ←→  GND
```

### Firmware Yükleme
```bash
# Raspberry Pi üzerinde
cd Rpi
./binFileUpdate F7Ap.bin
```

## Önemli Notlar

### F1 vs F7 Farkları

| Özellik | STM32F1 | STM32F7 |
|---------|---------|---------|
| Max Clock | 72 MHz | 216 MHz |
| Flash Yapısı | Sayfa (1-2KB) | Sektör (32-256KB) |
| Flash Yazma | HALFWORD (16-bit) | WORD (32-bit) |
| Cache | Yok | I-Cache + D-Cache |
| Overdrive | Yok | Var (>180MHz için) |
| Voltage Scale | Yok | Var (Scale 1,2,3) |

### Cache Kullanımı
F7'de cache'ler performans için kritiktir:
```c
// main() başında
CPU_CACHE_Enable();
```

### Flash Silme
F7'de sektör bazlı silme:
```c
EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
EraseInitStruct.Sector = FLASH_SECTOR_1;
EraseInitStruct.NbSectors = 7;
```

### Flash Yazma
F7'de word (32-bit) yazma:
```c
HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
```

## Hata Ayıklama

### LED Göstergeleri
- **Bootloader Modunda**: LED hızlı yanıp söner (data alımı)
- **Application Modunda**: LED 500ms aralıklarla yanıp söner

### UART Debug Mesajları
USART2 üzerinden debug mesajları:
```
STM32F7 Bootloader Started...
Transfer 1024
Transfer 2048
...
Boot Finished...
Jumping to Application...
STM32F7 Application v1.0 Started!
Running at 216MHz with I-Cache and D-Cache enabled
```

### Yaygın Sorunlar
1. **Application başlamıyor**:
   - Vector table doğru mu? (0x08008000)
   - Linker script doğru mu?

2. **Flash yazma hatası**:
   - Voltage range doğru mu? (FLASH_VOLTAGE_RANGE_3)
   - Sektörler silinmiş mi?

3. **Clock sorunları**:
   - Overdrive aktif mi?
   - PLL değerleri doğru mu?

## Güvenlik Notları

1. Flash yazma sırasında kesinti olmamalı
2. Application başlamadan önce doğrulama yapılmalı
3. Bootloader alanı korunmalı (write protect)

## Lisans

Copyright (c) 2024 STMicroelectronics.
Tüm hakları saklıdır.

## İletişim

Sorular ve öneriler için issue açabilirsiniz.
