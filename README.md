# Tactical Radar - 3D Cyber Parkour & Drone Combat Arena

Bu proje, **C++17** ve **raylib** kütüphanesi kullanılarak geliştirilmiş 3D platform, parkour, dinamik 3D drone desteği ve veri odaklı bölüm tasarımına sahip bir aksiyon/toplama (collectathon) oyunudur. Oyuncu, kamera açılarına duyarlı hareket mekanizmaları, dash yeteneği, 2D mini harita (radar), prosedürel ses sentezleme engine'i, çift modlu Drone desteği, düşman yapay zekası, Seviye Atlama / Yetenek Ağacı ve kaydedip yükleme (Save/Load) sistemleriyle donatılmış dinamik bir dünyada mücadele eder.

---

## 🎯 Projenin Genel Özellikleri

1. **🛸 Çift Modlu Drone Sistemi (Drone Companion & Recon System)**
   - **Dinamik 3D Model Yükleyici**: `resources/drone/drone.obj` ve `texture.png` mevcutsa 3D model otomatik olarak yüklenir. Model eklenmemişse sistem çökmez; 4 dönen rotörlü, gövdeli ve parlayan lazer gözlü prosedürel 3D Quadcopter çizer.
   - **Mod 1: Otomatik Takip & Lazer Saldırısı (`DRONE_MODE_FOLLOW_ATTACK`)**: Drone oyuncuyu smooth LERP fiziği ile takip eder. Menzildeki düşmanlara kilitlenerek kırmızı plazma lazer atışı yapar ve düşmanları imha eder (25 Hasar / atış).
   - **Mod 2: Serbest Uçuş & Keşif (`DRONE_MODE_FREE_FLIGHT`)**: `E` tuşu ile aktif edilir. Kamera ve kontroller Drone'a geçer. Oyuncu WASD, Space/Shift ve Fare ile 3D haritada serbestçe uçabilir.
   - **Radar HUD Entegrasyonu**: Drone'un konumu ve aktif modu mini haritada canlı Magenta / Lime renginde görüntülenir.

2. **⚔️ Drone Test Arenası & Sürekli Düşman Spawnerı (`F6` Tuşu)**
   - Oyuncunun `F6` tuşuna basarak veya menüden erişebileceği özel test haritası (`resources/map/level_test.txt`).
   - Her 3 saniyede bir otomatik yeni düşman doğuran Spawner mekanizması (drone lazer saldırılarını ve serbest uçuşu test etmek için).
   - Düşman sağlık barı (`Enemy::hp`) ve 3D uzayda yüzen görsel can göstergesi.

3. **3D Kamera & Karakter Kontrolü**
   - **Kamera Yönlü Hareket (Camera-Relative WASD)**: Trigonometrik hesaplamalar (`sin/cos`) ile kameranın baktığı açıya (`cameraYaw`) duyarlı hareket mantığı.
   - **Dash (Atılma) Mekaniği**: `Left Shift` tuşu ile 0.2 saniyelik yüksek hızlı ileri atılma, 2.0 saniye cooldown ve özel parçacık efekti/ekran sarsıntısı.
   - **Zıplama & Çift Zıplama (Double Jump)**: Yerçekimi, platform çakışma tespiti ve Seviye Atlama ekranından açılabilen çift zıplama yeteneği.
   - **Dinamik Kamera & Görüş Açısı (Dynamic FOV & Screen Shake)**: Dash esnasında FOV 45°'den 58°'ye esner; hasar alma ve atılma durumlarında ekran sarsıntısı uygulanır. `T` tuşu ile stratejik kuş bakışı görünüm.

4. **Oyun Durum Makinesi (Game State Machine)**
   - `STATE_MENU`: Ana menü, test arenası kısayolu (`F6`) ve kayıtlı oyun yükleme ekranı (`Enter` ile başlar, `L` ile yükler).
   - `STATE_PLAYING`: Gerçek zamanlı 3D aksiyon, fizik, drone takibi/uçuşu, düşman yapay zekası ve UI katmanı.
   - `STATE_LEVEL_UP`: Tecrübe barı dolduğunda oyunu duraklatıp yetenek seçim modalını (`[1] Speed Boost`, `[2] Rapid Dash`, `[3] Double Jump`, `[4] Max HP & Refill`) sunar.
   - `STATE_GAME_OVER`: Oyuncunun canı sıfırlandığında veya `Y = -15` seviyesinin altına düştüğünde tetiklenir (`R` tuşu ile yeniden başlatma).
   - `STATE_VICTORY`: Bütün bölümler tamamlandığında açılan zafer ve istatistik ekranı.

5. **Taktiksel Mini Harita (Radar HUD)**
   - Ekranın sağ üst köşesinde 2D dairesel radar retikülü.
   - 3D dünya koordinatlarının $(X, Z)$ radar yarıçapına oranlanması (Mapping).
   - **Oyuncu Göstergesi**: Cyan renkli nokta ve kameranın baktığı yönü gösteren vektör çizgisi (Lime).
   - **Drone Göstergesi**: Parlayan Magenta (Takip) veya Lime (Serbest Uçuş) renkli özel simge.
   - **EXP Kutuları**: Parlayan sarı/altın renkli noktalar.
   - **Düşmanlar**: Devriye modunda kırmızı, takip modunda yanıp sönen kırmızı/turuncu noktalar.

6. **Veri Odaklı Bölüm Tasarımı (Data-Driven Map Loader)**
   - Metin dosyalarından (`resources/map/level1.txt`, `level2.txt`, `level3.txt`, `level_test.txt`) 3D blok ve engel haritaları yükleme.

7. **Prosedürel Ses Engine'i (Procedural Synth Engine)**
   - C++ belleğinde `Wave` tamponları ile 8-bit / retro ses sentezleme (Dash, Collect, Jump, Hurt/Laser, LevelUp, Click).

8. **3D Parçacık Sistemi (Particle System & Visual Juice)**
   - Kutuları toplarken altın/cyan patlamaları, Dash duman izleri, yere iniş tozu ve lazer hasar patlamaları.

9. **Kayıt ve Yükleme Sistemi (Save & Load System)**
   - `savegame.dat` ikili serileştirme (`F5` Quick Save / `F9` Quick Load).

---

## 🏗 Mimari ve Modül Yapısı

```
Staj26/
├── main.cpp                  # Oyun ana döngüsü, durum makinesi, drone entegrasyonu ve UI
├── Drone.h / .cpp            # Çift modlu Drone (Takip/Saldırı & Serbest Uçuş), Lazer & Model Çizimi
├── Player.h / .cpp           # Oyuncu fiziği, hareket, can, tecrübe ve yetenek yükseltmeleri
├── CameraController.h / .cpp  # 3D üçüncü şahıs, Drone serbest uçuş kamerası ve sarsıntı
├── Radar.h / .cpp            # 2D mini harita HUD arayüzü (Oyuncu, Düşman ve Drone blipleri)
├── Map.h / .cpp              # Veri odaklı seviye yükleyici ve AABB fizik çakışması
├── Enemy.h / .cpp            # Düşman yapay zekası (Patrol/Chase), Can/Hasar sistemi
├── Collectible.h / .cpp      # 3D tecrübe kutuları, dönme/süzülme animasyonları
├── ParticleSystem.h / .cpp   # 3D parçacık efektleri (Lazer patlamaları, dash izleri)
├── AudioManager.h / .cpp     # Prosedürel ses sentezleme singleton motoru
├── SaveSystem.h / .cpp       # İkili (binary) oyun kaydetme ve yükleme sistemi
├── CMakeLists.txt            # Proje cmake yapılandırması
├── TODO.md                   # Geliştirme adımları ve tamamlanan aşamalar
└── resources/
    ├── character/            # Oyuncu 3D modeli ve kaplaması
    ├── drone/                # (Opsiyonel) Drone 3D modeli (drone.obj & texture.png)
    └── map/                  # Metin tabanlı haritalar (level1..3.txt & level_test.txt)
```

---

## 🧩 Sınıf Sorumlulukları (Detailed Class Reference)

### 1. `Drone` ([Drone.h](file:///Users/kuzeyersoy/Staj26/Drone.h) / [Drone.cpp](file:///Users/kuzeyersoy/Staj26/Drone.cpp))
- **İşlevi**: Drone yardımcısı ve keşif birimi.
- **Özellikler**: 3D model yükleme (`resources/drone/drone.obj`), fallback olarak 4 rotörlü prosedürel Quadcopter çizimi, oyuncuyu LERP ile süzülerek takip etme, otomatik en yakın düşman tespiti ve kırmızı plazma lazer atışları (25 DMG), `E` tuşu ile Serbest Uçuş moduna geçiş.

### 2. `Enemy` ([Enemy.h](file:///Users/kuzeyersoy/Staj26/Enemy.h) / [Enemy.cpp](file:///Users/kuzeyersoy/Staj26/Enemy.cpp))
- **İşlevi**: Tehlike birimleri.
- **Özellikler**: Waypoint devriye ve agresif takip yapay zekası, `hp` / `maxHp` takibi, `TakeDamage()` metodu ve baş üzerinde yüzen 3D can barı göstergesi.

### 3. `CameraController` ([CameraController.h](file:///Users/kuzeyersoy/Staj26/CameraController.h) / [CameraController.cpp](file:///Users/kuzeyersoy/Staj26/CameraController.cpp))
- **İşlevi**: Kamera yönetimi.
- **Özellikler**: Oyuncu takip kamerası, Drone Serbest Uçuş kamerası görüş açısı bağlamı, `T` tuşu ile Stratejik Üst Açı, smooth FOV ve ekran sarsıntısı.

---

## 🎮 Kontroller (Controls)

| Tuş | Eylem |
| :--- | :--- |
| **W / A / S / D** | Oyuncu Hareketi / Serbest Uçuşta Drone Yönlendirmesi |
| **E** | Drone Modunu Değiştir (Otomatik Saldırı ⟷ Serbest Uçuş) |
| **T** | Drone Test Arenası Haritasına Geçiş Yap (Sürekli Düşman Spawnerı) |
| **Space** | Zıplama (Oyuncu) / Yukarı Yükselme (Drone Uçuşunda) |
| **C** veya **Sol Shift** | Aşağı Alçalma (Drone Uçuşunda) / Dash (Oyuncuda) |
| **Fare Hareketi** | Kamerayı / Drone Bakış Yönünü Döndürme |
| **V** | Stratejik Kuş Bakışı Kamera Görünümünü Aç/Kapat |
| **H** | Kontrol Yardım Penceresini Göster (Basılı Tutulurken) |
| **F5** | Oyunu Hızlı Kaydet (Quick Save) |
| **F9** | Oyunu Hızlı Yükle (Quick Load) |
| **1 / 2 / 3 / 4** | Level Up Ekranında Yetenek Seçimi |
| **R** | Kamerayı Resetleme veya Game Over Durumunda Restart |
| **Enter** | Menüden Oyuna Başlama / Zafer Ekranında Menüye Dönme |
| **L** | Menü Ekranında Kayıtlı Oyunu Yükleme |

---

## 🛠 Derleme ve Çalıştırma (Build & Run)

```bash
# 1. Yapılandırma ve Derleme Dosyalarını Oluşturma
cmake -B build -S .

# 2. Projeyi Derleme
cmake --build build

# 3. Oyunu Çalıştırma
./build/Staj26
```