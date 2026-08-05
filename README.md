# Tactical Radar - 3D Cyber Parkour & Military Drone Logistics Simulation

Bu proje, **C++17** ve **raylib** kütüphanesi kullanılarak geliştirilmiş 3D platform, parkour, 5-drone'lu askeri otonom filo lojistik simülasyonu ve veri odaklı bölüm tasarımına sahip bir aksiyon/simülasyon oyunudur.

---

## 🪖 PART 1: Askeri Drone Filo Simülasyonu & Lojistik Arena (`F7` Modu)

> [!NOTE]
> **Simülasyon Durumu ve Taslak Yapı (Prototype / Draft Simulation)**
> Bu askeri filo simülasyonu şu anda temel mantıksal hatları (Recon, Task Force, Support rolleri, FSM durum makineleri, ikmal ve formasyon mekanizmaları) kurulmuş **taslak bir simülasyondur (prototype simulation)**. Gelecek aşamalarda daha fazla drone kapasitesi, sürü zekası ve algoritma/fizik optimizasyonları eklenecektir.

Ana menüden veya oyun içerisinden **`F7` Tuşu** ile erişilen bu mod, 5 otonom askeri drone'un $1000 \times 1000$ metrelik operasyon sahasında yürüttüğü gerçek zamanlı hedef tespiti, taktiksel yelpaze kuşatması, angajman, ikmal ve lojistik koordinasyonunu simüle eder.

### 🛸 5 Drone'lu Otonom Filo ve Görev Rolleri

1. **1x RECON Drone (`Skyblue` - Keşif & Hedef Tespit Unit)**:
   - Geniş operasyon sahasında ($1000 \times 1000$m) devriye gezerek henüz tespit edilmemiş düşman kümelerini (`UNDETECTED`) arar.
   - Hedef kümeye yaklaşarak 1.8 saniyelik tarama protokolünü tamamlar, kümenin GPS koordinatlarını ve düşman sayısını merkeze bildirir (`REPORTED`).

2. **3x TASK FORCE Drones (`Orange` - Taktiksel Önleme & Angajman Unitleri)**:
   - Bildirilen hedef kümelerine intikal eder.
   - **Taktiksel Yelpaze Kuşatması (Flanking Formation)**: Saldırı esnasında çakışmamak ve birbirini engellememek için hedef etrafında kademeli açılarda ($-40^\circ, 0^\circ, +40^\circ$), farklı mesafelerde ($34\text{m}, 40\text{m}, 46\text{m}$) mevkilenirler.
   - Hedefteki canlı düşman birimlerine kırmızı plazma lazer ateşler (40 DMG/sn). Yakıt veya mühimmatı %20'nin altına düştüğünde havadan ikmal çağrısı (`REQUESTING_SUPPORT`) yayınlar.

3. **1x SUPPORT Drone (`Lime` - Hava Lojistik & Ağır İkmal Cargo Unit)**:
   - Sahadaki Task Force drone'larının yakıt/mühimmat durumlarını anlık izler.
   - İkmal çağrısı yapan drone'a yönelerek onun $6\text{m}$ üstünde ve $5\text{m}$ arkasında formasyon uçuşuna geçer.
   - Yeşil ikmal tüzük bağı (tether beam) ile havada yakıt (+20/sn) ve mühimmat (+30/sn) aktarır.

### ⚡ Gerçekçi Uçuş Fiziği & Düşman Kaçış Yapay Zekası

- **Askeri Uçuş Fiziği**: İvmeli uçuş vektörü (`velocity`), ivmelenirken burnu öne eğme (`pitch` tilt) ve dönüşlerde yumuşak süzülme dinamikleri.
- **Boids Anti-Collision**: Drone'ların havada birbiriyle çakışmasını engelleyen 9m yarıçaplı itme kuvveti.
- **Düşman Panik & Kaçış Yapay Zekası**: Lazer ateşi altına giren veya arkadaşı vurulan düşmanlar panik haline (`panicTimer = 3.0s`) geçerek 7.5 m/s hızla lazer etki alanından kaçışır, zig-zag çizerek siper arar.

### 🎥 İnteraktif Kamera & HUD Kontrolleri

- **`0` veya `C`**: **Serbest Komuta Kamerası (Free Fly Cam)**. `WASD` ile hareket, `Space` / `Ctrl` ile irtifa, `Shift` ile turbo hız, `Fare` ile $360^\circ$ bakış.
- **`1` .. `5`**: İlgili drone'a kilitlenir. Kilitliyken **Fare Hareketi ile $360^\circ$ etrafı inceleme** imkanı sunar.
- **`TAB`**: Kameralar arası sırayla geçiş.
- **`F11`**: Tam Ekran (Fullscreen) modu.

### 🚀 Gelecek Geliştirme Haritası & Optimizasyon Planı (Future Roadmap)

1. **🐝 Sürü Hareketleri (Swarm Intelligence & Dynamics)**:
   - Reynolds Flocking algoritmaları (Cohesion, Alignment, Separation) entegrasyonu ile drone'ların sürü halinde kolektif karar verip hareket etmesi.
2. **🛸 Daha Fazla Drone Kapsamı (Multi-Squad Scaling)**:
   - Filo kapasitesinin 5 drone'dan 20+ drone'a çıkarılarak çoklu filo operasyonlarının simüle edilmesi.
3. **⚙️ Algoritmalarda Optimizasyon (Algorithmic & Graph Optimizations)**:
   - **Gelişmiş Hedef Atama**: Hungarian / Munkres algoritması ile drone-hedef eşleştirmelerinin optimize edilmesi.
   - **3D Yol Bulma (Pathfinding)**: $A^*$ / $RRT^*$ algoritmaları ile karmaşık 3D engeller etrafında dinamik rota planlama.
4. **💨 Drone Hareketlerinde Optimizasyon (Flight Trajectory Optimization)**:
   - Bézier / Spline yörünge yumuşatması ile gerçekçi aerodinamik dönüşler.
   - RVO (Reciprocal Velocity Obstacles) ile anlık engelden kaçınma algoritmaları.

---

## 🏗 PART 2: Proje Mimarisi, Dosya Yapısı & Kod Referansı

### 📁 Yeni Eklenen Modüller ve Dosyalar

- **[DroneSystem.h](file:///Users/kuzeyersoy/Staj26/DroneSystem.h)**: 5-Drone Fleet lojistik mimarisinin `DroneFleet` namespace altında tanımlanan tüm struct (`Drone`, `EnemyCluster`, `Enemy`, `HomeBase`, `TelemetryContext`) ve `DroneSystemManager` sınıf deklarasyonu.
- **[DroneSystem.cpp](file:///Users/kuzeyersoy/Staj26/DroneSystem.cpp)**: 5-Drone otonom FSM, taktiksel formasyon fiziği, Boids anti-collision, düşman kaçış yapay zekası, $1000 \times 1000$ harita çizimi ve 2D Telemetri HUD render motoru.

### 🧩 Tam Proje Dosya Dizini

```
Staj26/
├── main.cpp                  # Oyun ana döngüsü, durum makinesi (F7 Askeri Sim entegrasyonu)
├── DroneSystem.h / .cpp       # [YENİ] 5-Drone Otonom Filo, Lojistik, FSM, Formasyon & Telemetri
├── Drone.h / .cpp            # Oyuncu tekli yardımcısı drone (Takip/Saldırı & Serbest Uçuş)
├── Player.h / .cpp           # Oyuncu fiziği, hareket, can, tecrübe ve yetenek yükseltmeleri
├── CameraController.h / .cpp  # 3D üçüncü şahıs ve kamera sarsıntı motoru
├── Radar.h / .cpp            # 2D dairesel mini harita HUD arayüzü
├── Map.h / .cpp              # Veri odaklı metin tabanlı seviye yükleyici
├── Enemy.h / .cpp            # Düşman devriye & takip yapay zekası
├── Collectible.h / .cpp      # 3D tecrübe kutuları
├── ParticleSystem.h / .cpp   # 3D parçacık efektleri (Lazer, dash, patlamalar)
├── AudioManager.h / .cpp     # Prosedürel retro ses sentezleme motoru
├── SaveSystem.h / .cpp       # İkili (binary) oyun kaydetme sistemi
├── CMakeLists.txt            # Proje derleme yapılandırması
├── TODO.md                   # Geliştirme adımları ve tamamlanan aşamalar
└── resources/                # 3D modeller ve metin tabanlı haritalar
```

---

## 🎮 Kontroller (Controls Reference)

| Tuş | Eylem |
| :--- | :--- |
| **F7** | **Askeri Drone Filo Simülasyonu Moduna Geçiş Yap** |
| **F11** | Tam Ekran (Fullscreen) Modunu Aç / Kapat |
| **1 / 2 / 3 / 4 / 5** | Filo Simülasyonunda İlgili Drone'a Kilitlen (1: Recon, 2-4: Task Force, 5: Support) |
| **0** veya **C** | Filo Simülasyonunda Serbest Komuta Kamerasına Geçiş Yap |
| **TAB** | Kameralar Arası Sırayla Geçiş Yap |
| **Fare Hareketi** | Kamera Görüş Yönünü / Drone Takibinde $360^\circ$ Etrafı İnceleme |
| **W / A / S / D** | Karakter Hareketi / Serbest Kamerada Haritada Uçuş |
| **Space** | Zıplama (Oyuncu) / Yükselme (Serbest Kamerada) |
| **Left Control** | Alçalma (Serbest Kamerada) |
| **Left Shift** | Dash (Oyuncu) / Turbo Hızlı Uçuş (Serbest Kamerada) |
| **E** | Ana Oyunda Oyuncu Drone Modunu Değiştir (Saldırı ⟷ Serbest Uçuş) |
| **T** | Drone Test Arenası Haritasına Geçiş Yap |
| **F5 / F9** | Oyunu Hızlı Kaydet / Hızlı Yükle |
| **ESC** | Simülasyondan Ana Menüye Dönüş |

---

## 🛠 Derleme ve Çalıştırma (Build & Run)

```bash
# 1. CMake Yapılandırması
cmake -B build -S .

# 2. Projeyi Derleme
cmake --build build

# 3. Çalıştırma
./build/Staj26
```