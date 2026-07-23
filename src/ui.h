#pragma once
#include "downloader.h"

// حالات الواجهة
enum class AppState {
    MAIN_MENU,          // القائمة الرئيسية
    PREPARING,          // رسالة "Preparing your PS4..."
    DOWNLOADING,        // شاشة التحميل
    DONE,               // "All files downloaded successfully"
    ERROR_SCREEN,       // شاشة الخطأ
    SETTINGS,           // الإعدادات
    COMING_SOON,        // "Coming Soon..."
};

// مؤشر اختيار القائمة الرئيسية
enum class MenuSel {
    BTN_READY = 0,
    BTN_SOON  = 1,
};

class UI {
public:
    UI(Downloader& dl);

    // تحديث الواجهة (يُستدعى كل إطار)
    void update(uint32_t buttons, uint32_t prev_buttons);

    // رسم الواجهة
    void draw();

    // هل الخروج من التطبيق؟
    bool should_exit() const { return m_exit; }

private:
    Downloader&    m_dl;
    AppState       m_state;
    AppState       m_prev_state;
    MenuSel        m_menu_sel;
    DownloadStatus m_status;
    bool           m_exit;
    float          m_anim_t;      // وقت الأنيميشن (0..1)
    int            m_flash_timer; // للتأثير الوميض
    std::string    m_error_msg;

    // رسم الأجزاء
    void draw_background();
    void draw_header();
    void draw_main_menu();
    void draw_preparing();
    void draw_downloading();
    void draw_done();
    void draw_settings();
    void draw_coming_soon();
    void draw_error();
    void draw_footer_hint(const char* hint);

    // مساعد: رسم زر مع تأثير اختيار
    void draw_button(int x, int y, int w, int h,
                     const char* label, bool selected, bool disabled = false);

    // تبديل الحالة مع أنيميشن
    void transition_to(AppState next);

    // أزرار PS4
    static const uint32_t PAD_CROSS    = (1 << 14);
    static const uint32_t PAD_CIRCLE   = (1 << 13);
    static const uint32_t PAD_UP       = (1 << 4);
    static const uint32_t PAD_DOWN     = (1 << 6);
    static const uint32_t PAD_OPTIONS  = (1 << 3);
    static const uint32_t PAD_L1       = (1 << 10);
    static const uint32_t PAD_R1       = (1 << 11);

    bool just_pressed(uint32_t btn, uint32_t cur, uint32_t prev) {
        return (cur & btn) && !(prev & btn);
    }
};
