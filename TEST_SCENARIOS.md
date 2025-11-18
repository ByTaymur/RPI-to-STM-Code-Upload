# Test Senaryoları - RPI to STM32 Firmware Update

## 📋 Test Planı

Bu dokuman, sistemin tüm fonksiyonlarını test etmek için kapsamlı test senaryoları içerir.

---

## ✅ Test Kategorileri

1. [Birim Testleri](#birim-testleri)
2. [Entegrasyon Testleri](#entegrasyon-testleri)
3. [End-to-End Testleri](#end-to-end-testleri)
4. [Hata Senaryoları](#hata-senaryoları)
5. [Performance Testleri](#performance-testleri)
6. [Stress Testleri](#stress-testleri)

---

## 1. Birim Testleri

### Test 1.1: Checksum Hesaplama
**Amaç:** Checksum algoritmasının doğru çalıştığını doğrula

**Test Data:**
```c
uint8_t block[1024];
block[0] = 0x12;
block[1023] = 0xAB;

uint8_t checksum = (block[0] + block[1023]) & 0xFF;
// Beklenen: 0xBD (0x12 + 0xAB = 0xBD)
```

**Kontroller:**
- ✅ Checksum = 0xBD
- ✅ Overflow handling (0xFF + 0xFF = 0xFE)
- ✅ Zero bytes (0x00 + 0x00 = 0x00)

**Başarı Kriteri:** Tüm test durumları geçmeli

---

### Test 1.2: Handshake Packet Oluşturma
**Amaç:** RPI tarafında handshake paketinin doğru oluşturulduğunu doğrula

**Test Code:**
```c
uint32_t file_size = 12345;  // 0x00003039

uint8_t handshake[6];
handshake[0] = '{';
handshake[1] = (file_size >> 24) & 0xFF;  // 0x00
handshake[2] = (file_size >> 16) & 0xFF;  // 0x00
handshake[3] = (file_size >> 8)  & 0xFF;  // 0x30
handshake[4] = (file_size >> 0)  & 0xFF;  // 0x39
handshake[5] = '}';

// Beklenen: 7B 00 00 30 39 7D
```

**Kontroller:**
- ✅ Start byte = '{'
- ✅ Size doğru (big-endian)
- ✅ End byte = '}'

**Başarı Kriteri:** Paket formatı doğru

---

### Test 1.3: Handshake Parsing (STM32)
**Amaç:** STM32'nin handshake'i doğru parse ettiğini doğrula

**Test Input:**
```
RX: 7B 00 00 30 39 7D
```

**Beklenen Çıktı:**
```c
firmware_total_size = 12345
bootloader_state = STATE_RECEIVE_DATA
TX: 'O' (0x4F)
```

**Kontroller:**
- ✅ Size = 12345
- ✅ State değişti
- ✅ ACK gönderildi

---

### Test 1.4: Block Write Size Calculation
**Amaç:** Son block size'ın doğru hesaplandığını doğrula

**Test Senaryosu:**
```
Total size: 2500 bytes
Block 0: 1024 bytes
Block 1: 1024 bytes
Block 2: 452 bytes (2500 - 2048)
```

**Kod:**
```c
uint32_t remaining = firmware_total_size - bytes_received;
if(remaining >= MAX_BLOCK_SIZE)
    current_block_size = MAX_BLOCK_SIZE;
else
    current_block_size = remaining;
```

**Kontroller:**
- ✅ Block 0: size = 1024
- ✅ Block 1: size = 1024
- ✅ Block 2: size = 452

---

## 2. Entegrasyon Testleri

### Test 2.1: RPI-STM32 Handshake
**Amaç:** RPI ve STM32 arasında handshake başarılı olmalı

**Adımlar:**
1. STM32'yi bootloader moduna al
2. RPI'den handshake gönder
3. ACK bekle

**Beklenen:**
```
RPI  → STM32: {SIZE}
RPI ← STM32: 'O'
```

**Timeout:** 5 saniye

**Başarı Kriteri:** ACK alındı

---

### Test 2.2: Single Block Transfer
**Amaç:** Tek bir 1024 byte block başarıyla transfer edilmeli

**Adımlar:**
1. Handshake yap
2. 1024 byte gönder
3. Checksum bekle

**Test Data:**
```c
uint8_t block[1024];
memset(block, 0xAA, 1024);
block[0] = 0x12;
block[1023] = 0x34;
// Expected checksum: (0x12 + 0x34) & 0xFF = 0x46
```

**Kontroller:**
- ✅ Tüm byteslar gönderildi
- ✅ Checksum = 0x46
- ✅ STM32 flash'a yazdı

---

### Test 2.3: Multiple Block Transfer
**Amaç:** Birden fazla block transfer edilebilmeli

**Adımlar:**
1. 3 KB dosya oluştur (3 block)
2. Transfer et
3. Tüm checksumları doğrula

**Beklenen:**
```
Block 0: 1024 bytes → Checksum OK
Block 1: 1024 bytes → Checksum OK
Block 2: 1024 bytes → Checksum OK
Total: 3072 bytes
```

---

## 3. End-to-End Testleri

### Test 3.1: Tam Firmware Update
**Amaç:** Gerçek bir firmware dosyası baştan sona transfer edilmeli

**Test Firmware:**
```bash
# Basit blink uygulaması oluştur
cd Stm/Ap/F103Ap
make
arm-none-eabi-objcopy -O binary build/app.elf build/app.bin
```

**Adımlar:**
1. Bootloader'ı STM32'ye yükle
2. RPI'den firmware gönder
3. Transfer bitince application başlamalı

**Beklenen:**
```
[RPI] Handshake... OK
[RPI] Transferring... 100%
[RPI] Complete!
[STM32] Jumping to app...
[STM32] Application running!
```

---

### Test 3.2: LED Blink Verification
**Amaç:** Transfer edilen application çalışmalı

**Test Application:**
```c
// Simple blink @ 1Hz
while(1)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    HAL_Delay(500);
}
```

**Kontrol:**
- ✅ LED 1Hz'de yanıp sönüyor
- ✅ Application stabil çalışıyor

---

## 4. Hata Senaryoları

### Test 4.1: Checksum Mismatch Recovery
**Amaç:** Checksum hatası durumunda retry mekanizması çalışmalı

**Simülasyon:**
```c
// STM32'de checksum'ı kasıtlı olarak boz
checksum_response = correct_checksum + 1;
```

**Beklenen:**
```
[RPI] Block 0 sent
[RPI] Checksum mismatch!
[RPI] Retrying block 0... (1/5)
[RPI] Block 0 sent
[RPI] Checksum OK!
```

**Kontrol:**
- ✅ Retry yapıldı
- ✅ Block tekrar gönderildi
- ✅ İkinci denemede başarılı

---

### Test 4.2: Handshake Timeout
**Amaç:** STM32 ACK göndermezse timeout olmalı

**Simülasyon:**
1. STM32'yi resetle ama bootloader yükleme
2. RPI'den handshake gönder

**Beklenen:**
```
[RPI] Handshake...
[RPI] Waiting for ACK...
[RPI] ERROR: Handshake timeout (5000ms)
```

**Kontrol:**
- ✅ 5 saniye sonra timeout
- ✅ Hata mesajı görüntülendi
- ✅ Program sonlandı

---

### Test 4.3: UART Disconnect During Transfer
**Amaç:** Transfer sırasında bağlantı kesilirse hata yakalanmalı

**Simülasyon:**
1. Transfer başlat
2. 50% tamamlandığında UART kablosunu çek

**Beklenen:**
```
[RPI] Block 25/50 sent
[RPI] Checksum timeout!
[RPI] Retrying... (1/5)
[RPI] Retrying... (5/5)
[RPI] ERROR: Failed after 5 retries
```

---

### Test 4.4: Invalid File Size
**Amaç:** Geçersiz dosya boyutu reddedilmeli

**Test:**
```bash
# Çok büyük dosya (300KB)
dd if=/dev/urandom of=large.bin bs=1024 count=300
./binFileUpdate large.bin 1
```

**Beklenen:**
```
ERROR: File too large (307200 bytes). Max: 262144 bytes
```

---

### Test 4.5: Corrupted Binary File
**Amaç:** Bozuk binary dosya tespit edilmeli

**Test:**
```bash
# Boş dosya
touch empty.bin
./binFileUpdate empty.bin 1
```

**Beklenen:**
```
ERROR: File is empty
```

---

## 5. Performance Testleri

### Test 5.1: Transfer Speed Measurement
**Amaç:** Transfer hızını ölç

**Test Sizes:**
- 10 KB
- 48 KB
- 128 KB

**Metrik:**
```
Transfer Speed = File Size / Transfer Time
Target: ≥ 9 KB/s
```

**Ölçüm:**
```bash
time ./binFileUpdate app.bin 1
```

**Beklenen:**
| Boyut | Süre (max) | Throughput |
|-------|------------|------------|
| 10 KB | 2 saniye   | 5 KB/s     |
| 48 KB | 6 saniye   | 8 KB/s     |
| 128 KB| 15 saniye  | 8.5 KB/s   |

---

### Test 5.2: Memory Usage
**Amaç:** Bellek kullanımını kontrol et

**STM32 RAM:**
```c
// Stack
uint8_t data_block[1024];  // 1KB

// Heap
// None

// Total: ~1.5KB (includes other variables)
```

**Kontrol:**
- ✅ Stack overflow yok
- ✅ Heap fragmentation yok

---

## 6. Stress Testleri

### Test 6.1: Repeated Transfers
**Amaç:** Ardışık transferlerde bellek sızıntısı olmamalı

**Test:**
```bash
for i in {1..100}; do
    echo "Test $i/100"
    ./binFileUpdate app.bin 1
    sleep 1
done
```

**Kontrol:**
- ✅ Tüm transferler başarılı
- ✅ Bellek kullanımı sabit
- ✅ Performance degradation yok

---

### Test 6.2: Maximum Size Transfer
**Amaç:** Maksimum dosya boyutu transfer edilebilmeli

**Test:**
```bash
# 256 KB dosya
dd if=/dev/urandom of=max.bin bs=1024 count=256
./binFileUpdate max.bin 1
```

**Kontrol:**
- ✅ Transfer başarılı
- ✅ Süre < 30 saniye
- ✅ Checksum hataları yok

---

### Test 6.3: Long Cable Test
**Amaç:** Uzun UART kablosu ile test

**Setup:**
- 5 meter UART kablosu
- EMI kaynakları yakında

**Beklenen:**
- Retry'ler artabilir
- Ama sonuçta başarılı olmalı

---

## 📊 Test Raporu Şablonu

```markdown
## Test Raporu

**Tarih:** YYYY-MM-DD
**Tester:** İsim
**Platform:** RPI Model X + STM32F103

### Test Sonuçları

| Test ID | Test Adı | Sonuç | Not |
|---------|----------|-------|-----|
| 1.1 | Checksum | ✅ PASS | - |
| 1.2 | Handshake Packet | ✅ PASS | - |
| 2.1 | RPI-STM32 Handshake | ✅ PASS | - |
| 3.1 | Full Update | ✅ PASS | 12.5s |
| 4.1 | Checksum Recovery | ✅ PASS | 2 retry |
| ... | ... | ... | ... |

### Özet
- Toplam Test: 25
- Başarılı: 24
- Başarısız: 1
- Başarı Oranı: 96%

### Sorunlar
1. Test 4.3 - UART disconnect recovery bazen fail oluyor
   - Root cause: Timeout değeri çok kısa
   - Fix: Timeout 2000ms → 3000ms
```

---

## 🎯 Acceptance Criteria

Transfer sistemi aşağıdaki kriterleri sağlamalıdır:

### Fonksiyonel
- ✅ Handshake %100 başarılı
- ✅ Checksum doğrulama %100 doğru
- ✅ Retry mekanizması çalışıyor
- ✅ Flash yazma başarılı
- ✅ Application jump çalışıyor

### Performance
- ✅ Transfer hızı ≥ 8 KB/s
- ✅ Bellek kullanımı < 2 KB RAM
- ✅ CPU kullanımı < %50

### Güvenilirlik
- ✅ 100 ardışık transfer başarılı
- ✅ Error recovery rate > %95
- ✅ Checksum hata toleransı %100

### Kullanılabilirlik
- ✅ Progress bar çalışıyor
- ✅ Hata mesajları açıklayıcı
- ✅ Kullanım basit (2 parametre)

---

## 🚀 Automated Test Script

```bash
#!/bin/bash
# auto_test.sh - Automated test runner

TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

function run_test() {
    local test_name=$1
    local test_cmd=$2

    echo "Running: $test_name"
    TEST_COUNT=$((TEST_COUNT + 1))

    if eval "$test_cmd"; then
        echo "✅ PASS: $test_name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "❌ FAIL: $test_name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# Run tests
run_test "Small file (10KB)" "./binFileUpdate test_10k.bin 1"
run_test "Medium file (48KB)" "./binFileUpdate test_48k.bin 1"
run_test "Large file (128KB)" "./binFileUpdate test_128k.bin 1"

# Report
echo ""
echo "================ TEST REPORT ================"
echo "Total Tests: $TEST_COUNT"
echo "Passed:      $PASS_COUNT"
echo "Failed:      $FAIL_COUNT"
echo "Success Rate: $(( PASS_COUNT * 100 / TEST_COUNT ))%"
echo "============================================="
```

---

**Son Güncelleme:** 2024-11-18
**Versiyon:** 2.1
**Durum:** Ready for Testing
