# Tactical Radar - 3D Cyber Parkour & Military Drone Logistics Simulation

Bu proje, **C++17** ve **raylib** kütüphanesi kullanılarak geliştirilmiş 3D platform, parkour, 9-drone'lu askeri otonom filo lojistik simülasyonu ve veri odaklı bölüm tasarımına sahip bir aksiyon/simülasyon oyunudur.

---

## 📑 OPERASYONEL SENARYO REHBERİ (`Scenario.md`)

Simülasyondaki **tüm drone rollerini**, **düşman tiplerini** ve **otonom yapay zeka kararlarını** sade bir dille açıklayan dokümanımıza **[Scenario.md](file:///Users/kuzeyersoy/Staj26/Scenario.md)** dosyasından ulaşabilirsiniz.

---

## 🚀 SON YENİLİKLER VE GELİŞTİRMELER

1. **💪 Drone Canları 2 Katına Çıkarıldı**:
   - **Recon (Keşif)**: $160\text{ HP}$ (Eskiden 80 HP)
   - **Task Force (Saldırı)**: $200\text{ HP}$ (Eskiden 100 HP)
   - **Support (Destek)**: $300\text{ HP}$ (Eskiden 150 HP)
2. **⚡ Support Drone Hız Eşitlemesi (2x Hız = 16.0 m/s)**:
   - Destek drone'larının uçuş hızı Saldırı drone'ları ile eşitlendi ($16\text{ m/s}$). Artık taarruz gruplarını geriden takip etmeden yanlarında uçabiliyorlar.
3. **📍 Havada Orta Noktada İkmal Buluşması (Midpoint Rendezvous)**:
   - Yakıtı veya mühimmatı azalan Saldırı drone'ları dümdüz durup beklemek yerine merkeze doğru uçar ve Destek drone'u ile **tam orta noktada** buluşur.
4. **🚑 Kurtarma Görevi ve Enkaz Çevresi Temizliği (SAR Fix)**:
   - Vurulan drone'ların etrafında düşman varsa Saldırı drone'ları önce bölgeyi temizler. Düşman temizlendiğinde Destek drone'u yere inerek vurulan drone'u %100 tamir edip düştüğü yerden savaşa geri döndürür.
5. **🛡️ Silahlı Düşman Görevlerinde Zorunlu Destek Eşliği**:
   - Silahlı düşman grubuna görev çıktığında en az 1 Destek drone'u Saldırı filosuyla beraber zorunlu olarak havalanır ve çatışma boyunca havadan ikmal sağlar.
6. **⏱️ Zaman Kontrolü ve Manuel Test Tuşları**:
   - `P` (Pause), `+` / `-` (Hızlandır / Yavaşlat), `K` (Silahlı Düşman Ekle), `L` (Silahsız Düşman Ekle), `J` (Ağır Düşman Kümesi Ekle).

---

## 🪖 PART 1: Askeri Drone Filo Simülasyonu & Lojistik Arena (`F7` Modu)

Ana menüden veya oyun içerisinden **`F7` Tuşu** ile erişilen bu mod, 9 otonom askeri drone'un $1000 \times 1000$ metrelik operasyon sahasında yürüttüğü gerçek zamanlı hedef tespiti, taktiksel yelpaze kuşatması, angajman, ikmal ve lojistik koordinasyonunu simüle eder.

### 🛸 9 Drone'lu Otonom Filo ve Görev Rolleri

1. **2x RECON Drones (`Skyblue` - Keşif & Hedef Tespit Unitleri - #1, #2)**:
   - 4x Hız ($32\text{ m/s}$), $160\text{ HP}$, $140\text{m}$ Radar Yarıçapı.
   - Haritayı hızlıca tarayarak düşman gruplarını keşfeder ve Komuta Merkezine rapor sunar.
   - Yakıtı azaldığında Destek drone'u ile orta noktada buluşup ikmal alır.

2. **5x TASK FORCE Drones (`Orange` - Taktiksel Önleme & Angajman Unitleri - #3 .. #7)**:
   - 2x Hız ($16\text{ m/s}$), $200\text{ HP}$, Kırmızı Taarruz Lazeri ($40\text{ Dmg/s}$).
   - Düşman kümesinin hesaplanan Tehdit Magnitude değerine göre havalanır, hedefe kilitlenip kuşatma açısı alır.

3. **2x SUPPORT Drones (`Lime` - Hava Lojistik, İkmal & Kurtarma Unitleri - #8, #9)**:
   - 2x Hız ($16\text{ m/s}$), $300\text{ HP}$, Yeşil İkmal ve SAR Tamir Işını.
   - Havada ikmal yapar, vurulan drone'ları düşündüğü yerde tamir eder ve silahlı düşman operasyonlarına eşlik eder.

---

## 🎮 Kontroller (Controls Reference)

| Tuş | Eylem |
| :--- | :--- |
| **F7** | **Askeri Drone Filo Simülasyonu Moduna Geçiş Yap** |
| **P / SPACE** | **Simülasyonu Duraklat / Devam Ettir (Pause)** |
| **+ / -** | **Simülasyon Hızını Artır / Azalt (2x, 4x, 8x / 0.5x, 0.25x)** |
| **K** | **Manuel ARMED (Silahlı Kırmızı) Düşman Kümesi Ekle** |
| **L** | **Manuel UNARMED (Silahsız Turuncu) Düşman Kümesi Ekle** |
| **J** | **Manuel HEAVY MIXED (Ağır) Düşman Kümesi Ekle** |
| **1 .. 9** | Filo Simülasyonunda İlgili Drone'a Kilitlen (1-2: Recon, 3-7: Task Force, 8-9: Support) |
| **0** veya **C** | Filo Simülasyonunda Serbest Komuta Kamerasına Geçiş Yap |
| **TAB** | Kameralar Arası Sırayla Geçiş Yap |
| **M** | Tam Ekran Taktik Radar Haritasını Aç |
| **I** | Seçili Drone Telemetri Müfettişini Aç |

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