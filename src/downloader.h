#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <functional>

// معلومات ملف التحميل
struct DownloadFile {
    std::string name;   // اسم الملف
    std::string url;    // رابط التحميل
};

// حالة التحميل الحالية
struct DownloadStatus {
    std::string current_file;     // اسم الملف الحالي
    int         current_index;    // رقم الملف الحالي (0-based)
    int         total_files;      // إجمالي الملفات
    float       file_progress;    // نسبة الملف الحالي (0.0 - 1.0)
    float       total_progress;   // نسبة الإجمالية (0.0 - 1.0)
    double      speed_kbps;       // السرعة (KB/s)
    int         eta_seconds;      // الوقت المتبقي (ثانية)
    uint64_t    downloaded_bytes; // بايتات محملة
    uint64_t    total_bytes;      // إجمالي البايتات
    bool        finished;         // هل انتهى؟
    bool        error;            // هل في خطأ؟
    std::string error_msg;        // نص الخطأ
};

// نوع callback لتحديث الحالة
using ProgressCallback = std::function<void(const DownloadStatus&)>;

class Downloader {
public:
    Downloader();
    ~Downloader();

    // تحميل قائمة الملفات من downloads.json
    bool load_list(const char* json_path);

    // تحميل قائمة الملفات مباشرة من URL
    bool load_list_from_url(const char* url);

    // بدء التحميل (يعمل في thread منفصل)
    void start(ProgressCallback callback);

    // إيقاف التحميل
    void cancel();

    // هل التحميل شغّال؟
    bool is_running() const { return m_running; }

    // الحصول على حالة التحميل الحالية (thread-safe)
    DownloadStatus get_status();

    // الحصول على قائمة الملفات
    const std::vector<DownloadFile>& get_files() const { return m_files; }

    // مسار حفظ الملفات على PS4
    static const char* SAVE_PATH;

private:
    std::vector<DownloadFile> m_files;
    volatile bool             m_running;
    volatile bool             m_cancel;
    DownloadStatus            m_status;

    // تحميل ملف واحد
    bool download_single(const DownloadFile& file, int index, int total,
                         ProgressCallback& cb);

    // parse JSON يدوي بسيط (بدون مكتبة خارجية)
    bool parse_json(const std::string& json);

    // تنسيق الوقت
    static std::string format_eta(int seconds);
    static std::string format_size(uint64_t bytes);
    static std::string format_speed(double kbps);
};
