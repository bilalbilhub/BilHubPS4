# BilHubPS4 v1.0.0
**Developer: BilHub**

---

## هيكل المشروع

```
BilHubPS4/
├── Makefile                  ← أمر البناء
├── CMakeLists.txt            ← بديل CMake
├── downloads.json            ← قائمة الملفات (روابطك من archive.org)
├── sce_sys/
│   ├── icon0.png             ← أيقونة التطبيق (512×512) — أضفها أنت
│   └── README.txt
└── src/
    ├── main.cpp              ← مدخل التطبيق
    ├── renderer.h / .cpp     ← نظام الرسم 1920×1080
    ├── ui.h / .cpp           ← الواجهة الكاملة
    └── downloader.h / .cpp   ← نظام التحميل
```

---

## خطوات البناء (على جهازك)

### 1. ثبّت OpenOrbis Toolchain
```bash
git clone https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain.git
cd OpenOrbis-PS4-Toolchain
# اتبع تعليمات التثبيت المناسبة لنظامك (Windows/Linux/macOS)
```

### 2. اضبط متغير البيئة
```bash
# Linux / macOS
export OO_PS4_TOOLCHAIN=/path/to/OpenOrbis-PS4-Toolchain

# Windows (PowerShell)
$env:OO_PS4_TOOLCHAIN = "C:\OpenOrbis-PS4-Toolchain"
```

### 3. أضف أيقونة التطبيق
- ضع ملف `icon0.png` (512×512) داخل مجلد `sce_sys/`
- يمكنك صنع واحدة بسيطة بـ Photoshop أو أي برنامج رسم

### 4. ابنِ المشروع
```bash
cd BilHubPS4
make
```

### 5. ستجد الناتج هنا:
```
build/BilHubPS4.pkg  ← هذا ما تحمّله على PS4
```

---

## تثبيت BilHubPS4.pkg على PS4

### الطريقة الأسهل: عبر الشبكة
```
1. على الكمبيوتر: شغّل "PS4 PKG Sender" أو "goldhen_pkg_sender"
2. ضع BilHubPS4.pkg فيه
3. على PS4 (بعد فتح GoldHEN / HEN):
   - Debug Settings > Game > Package Installer
   - أو استخدم Itemzflow Store
4. اكتب IP الكمبيوتر وثبّت منه
```

### الطريقة عبر FTP
```
1. شغّل FTP على PS4 (من GoldHEN)
2. الـ IP يظهر على شاشة PS4
3. باستخدام FileZilla أو WinSCP:
   انقل BilHubPS4.pkg إلى /data/
4. من Package Installer ثبّته
```

---

## كيف يعمل التطبيق بعد التثبيت؟

```
1. افتح BilHubPS4 من قائمة PS4
2. اضغط X على "Let Your PS4 Ready For Use"
3. يبدأ التطبيق بتحميل الملفات من archive.org:

   FPKGi Data.pkg         ✓
   FPKGi.pkg              ✓
   PS4_APOL00004_v2.3.2.pkg ✓
   PS4_CHTM00777_v1.2.2.pkg ✓
   PS4_CUSA00127_v1.53.pkg  ✓
   PS4_ITEM00001_v1.08.pkg  ✓
   PS4_LAPY20009_v2.07.pkg  ✓
   app.pkg                ✓

4. بعد الانتهاء تظهر رسالة:
   "All files downloaded successfully."

5. الملفات تُحفظ في: /data/BilHubPS4/downloads/
6. افتح Package Installer وثبّتها من هناك
```

---

## تحديث قائمة الملفات بدون إعادة بناء الـ PKG

```
1. عدّل ملف downloads.json على الكمبيوتر
2. عبر FTP ارفعه إلى PS4:
   /data/BilHubPS4/downloads.json
3. التطبيق يقرأ النسخة الخارجية تلقائياً
```

---

## ملفاتك الحالية في archive.org

| الملف | الحجم |
|-------|-------|
| FPKGi Data.pkg | 46 MB |
| FPKGi.pkg | 80 MB |
| PS4_APOL00004_v2.3.2.pkg | 21 MB |
| PS4_CHTM00777_v1.2.2.pkg | 18 MB |
| PS4_CUSA00127_v1.53.pkg | 61 MB |
| PS4_ITEM00001_v1.08.pkg | 25 MB |
| PS4_LAPY20009_v2.07.pkg | 60 MB |
| app.pkg | 22 MB |

**الإجمالي: ~333 MB**
