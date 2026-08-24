# Özerk Drone Filosu - Senaryo ve Rehber Dokümanı (`Scenario.md`)

Bu doküman, simülasyondaki **drone'ların**, **düşmanların** ve **tüm olayların (senaryoların)** ne anlama geldiğini ve yapay zekanın nasıl karar verdiğini anlatmaktadır.

---

## 1. Bizim Drone'larımız (Toplam 9 Tane - Canlar 2 Katına Çıkarıldı)

Filomuzda 3 farklı görev üstlenen **9 adet drone** bulunmaktadır:

1. **Recon (Keşif) Drone'ları (2 Tane - #1 ve #2)**:
   - **Görevi**: Haritayı hızlıca gezip düşman gruplarını bulmak ve merkeze raporlamaktır.
   - **Özellikleri**: Çok hızlıdırlar (4x Hız - $32\text{ m/s}$), depoları büyüktür. **Canları $160\text{ HP}$'ye çıkarılmıştır.** Silahları yoktur.

2. **Task Force (Saldırı) Drone'ları (5 Tane - #3, #4, #5, #6, #7)**:
   - **Görevi**: Keşfedilen düşman gruplarını vurmak ve yok etmektir.
   - **Özellikleri**: Hızlıdırlar (2x Hız - $16\text{ m/s}$), kırmızı lazerle ateş ederler. **Canları $200\text{ HP}$'ye çıkarılmıştır.**

3. **Support (Destek / İkmal) Drone'ları (2 Tane - #8 ve #9)**:
   - **Görevi**: Yakıtı veya mermisi biten drone'ları havada beslemek ve vurulan drone'ları düşündüğü yerde tamir edip çalıştırmaktır.
   - **Özellikleri**: **Hızları Saldırı Drone'ları ile eşitlenmiştir (2x Hız - $16\text{ m/s}$)**. **Canları $300\text{ HP}$'ye çıkarılmıştır.** Dev gibi depolarında ekstra yakıt ve mühimmat taşırlar.

---

## 2. Düşman Tipleri ve Tehdit Seviyesi Hesaplama

Düşmanlar 2 gruba ayrılır. Merkez, haritadaki düşman sayısına göre kaç tane saldırı drone'u göndereceğini hesaplar:

- **🔴 Silahlı Düşmanlar (Koyu Kırmızı Küpler)**:
  - Bizim drone'larımıza **kırmızı mermilerle ateş ederler** ve hasar verirler.
  - **Tehdit Gücü**: Her 2 silahlı düşman için **1 Saldırı Drone'u** görevlendirilir (Hesap: Düşman Sayısı $\times 0.5$).
- **🟠 Silahsız Düşmanlar (Turuncu Küpler)**:
  - Ateş etmezler. Ateş yediklerinde panikleyip sağa sola kaçışırlar.
  - **Tehdit Gücü**: Her 4 silahsız düşman için **1 Saldırı Drone'u** görevlendirilir (Hesap: Düşman Sayısı $\times 0.25$).

---

## 3. Arayüz ve Bilgilendirme Pencereleri (YENİ!)

1. **🏷️ Düşman Üstü Bilgi Etiketleri (3D/2D Floating Labels)**:
   - Keşfedilen her düşman grubunun tepesinde **Düşman Sayısı**, **Tehdit Magnitude'u** ve **Silahlı / Silahsız (Armed/Unarmed)** bilgisi canlı olarak yazar. (Örn: `CLUSTER #1: 8 UNITS (ARMED) | Threat Mag: 4`).
2. **📋 Aktif Görevler Penceresi (Active Task Force Missions Panel)**:
   - Sahadaki aktif taarruz görevlerini gösterir.
   - Hangi düşman kümesine kaç adet Saldırı (`TF #3,#4`) ve Kaç adet Destek (`SUP #8`) drone'unun atandığını gösterir.
   - **Düşman kümesi yok edilip temizlendiğinde görev bu pencereden otomatik olarak silinir!**
3. **📟 Büyütülmüş Komuta Terminali (Expanded Operational Terminal)**:
   - Terminal genişliği **`540px`**, yüksekliği **`380px`** olarak büyütüldü. Aynı anda 20 satır detaylı yapay zeka kararı görüntülenebilir.

---

## 4. Olaylar ve Drone Yapay Zekası Senaryoları

---

### Senaryo 1: Düşman Arama ve Keşif
- Keşif drone'ları haritadaki kareleri tur atarak gezer.
- Etraflarında mavi renkli bir radar çemberi vardır ($140\text{m}$).
- Bir düşman grubu gördüklerinde yaklaşıp dururlar ve merkeze haber verirler.

---

### Senaryo 2: Havada Yakıt İkmali (Merkeze Dönmeden)
- Keşif drone'unun yakıtı azaldığında (%30'a düştüğünde) merkeze kadar dönüp zaman kaybetmez.
- Boşta bir Destek drone'u var ise sinyal atar. Yok ise merkeze yakıt ikmali için döner.
- İki drone **havada orta noktada** buluşur. Destek drone'u yeşil ışınla Keşif drone'unun yakıtını %100 yapar.
- Keşif drone'u üsse hiç uğramadan **havadan arama işlemine kaldığı yerden devam eder**.

---

### Senaryo 3: Saldırıya Çıkış ve Kuşatma
- Merkez düşmanın gücüne göre kaç drone gerekiyorsa (örneğin 3 tane) o kadar Saldırı drone'unu aynı anda havalandırır.
- Drone'lar düşmanın etrafında daire çizerek birbiriyle çarpışmadan kuşatma açısı alırlar ve kilitlenip ateş ederler.

---

### Senaryo 4: Çok Düşman Gelirse (12 Silahlı Düşman Durumu)
- Düşman sayısı çok fazla olduğunda (örneğin 12 silahlı düşman = 6 Tehdit Seviyesi), elimizdeki 5 Saldırı drone'unun yetmeyeceği durumlar oluşur.
- **Yapay Zeka Çözümü**:
  1. Elimizdeki 5 Saldırı drone'unun hepsi havalanır.
  2. Destek drone'ları da **"Saha Sıhhiyesi"** olarak hemen arkalarından gider.
  3. Çatışma sırasında canı azalan veya mermisi biten drone'lara Destek drone'u havada anında can ve mermi yükler.
  4. İlk dalgada düşmanın bir kısmı yok edilince, kalan drone'lar 2. dalga ile temizliği tamamlar.

---

### Senaryo 5: Vurulan Drone'u Sahada Kurtarma
- Düşmanların ateşiyle bir Saldırı drone'u vurulup düşerse, Destek drone'u 1. öncelik olarak enkaz bölgesine uçar.
- Yeşil tamir ışınını bağlayarak vurulan drone'u düştüğü yerde %100 tamir eder ve tekrar çalıştırır.
- Vurulan drone **merkeze gitmeden düştüğü yerden kalkıp savaşa devam eder**.

---

### Senaryo 6: Havadayken U Dönüşü & Orta Noktada İkmal Buluşması
- Görevden dönen drone'lar yoldayken yeni bir düşman keşfedilirse, drone'lar depolarındaki yakıt ve mermiyi kontrol eder.
- Yeterli kaynakları varsa merkeze kadar inmeyip **havadayken anında U dönüşü yaparak** yeni hedefe saldırırlar.
- Yeterli kaynakları yoksa dümdüz beklemek yerine Destek drone'u ile **orta noktada buluşup** havada ikmal alırlar. Diğer hazır saldırı drone'ları da takım arkadaşlarını beklemeden tek başına saldırmaz.

---

### Senaryo 7: Kurtarma Görevi (Enkaz Çevresi Temizliği)
- Düşmanlar tarafından vurulan drone için Keşif drone'u hedefin koordinatlarını merkez binaya bildirir.
- Enkaz alanının yakınında düşman birlikleri var ise Saldırı drone'ları ve Destek drone'ları beraber operasyona çıkar.
- Saldırı drone'ları enkaz çevresindeki düşmanları temizler, ardından Destek drone'u enkaz alanına inerek kurtarma görevini başlatır.
- Eğer enkaz alanında düşman yoksa Destek drone'u doğrudan gidip kurtarma görevini başlatır.

---

### Senaryo 8: Silahlı Düşman Görevlerinde Zorunlu Destek Eşliği (YENİ!)
- Keşif drone'u **Silahlı Düşman Grubu** raporladığında, Saldırı filosu havalanırken **en az 1 adet Destek Drone'u zorunlu olarak onlarla beraber yola çıkar**.
- Destek drone'u çatışma boyunca Saldırı filosunun yanında durarak sürekli mühimmat ve yakıt ikmali sağlar.

---

## 5. Kolay Test Tuşları

- **`P` veya `SPACE`**: Simülasyonu Durdurur / Başlatır.
- **`+` / `-`**: Simülasyon Hızını Artırır / Azaltır ($2x \to 4x \to 8x$).
- **`F7` / `G`**: Taktik Görev Sonu Değerlendirme & Skor Karnesi (Debriefing Panel).
- **`X`**: Seçili drone'dan Magnezyum Savunma Fişeği (Flare) fırlatır.
- **`F6` / `V`**: Canlı PiP Mini Burun Kamerasını açar / kapatır (`[` / `]` ile hedef seçilir).
- **`T` / `N`**: FLIR Termal (White-Hot) / NVG Gen-3 Yeşil Fosfor Gece Görüş Vizörleri.
- **`K`**: Ekrana **Silahlı (Kırmızı)** Düşman Grubu ekler.
- **`L`**: Ekrana **Silahsız (Turuncu)** Düşman Grubu ekler.
- **`J`**: Ekrana **Ağır Karışık Düşman Grubu** ekler.
- **`TAB` / `0..9`**: Drone'ların gözünden bakmak için kameraları değiştirir.
- **`I`**: Seçili drone'un detaylı Telemetri ve Uçuş Denetçisi panelini açar.