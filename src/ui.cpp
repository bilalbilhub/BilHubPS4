#include "ui.h"
#include "renderer.h"
#include <stdio.h>
#include <string.h>

// ======================== مساعد لتنسيق الأرقام ========================

static std::string fmt_speed(double kbps) {
    char buf[64];
    if (kbps >= 1024.0)
        snprintf(buf, sizeof(buf), "%.1f MB/s", kbps / 1024.0);
    else
        snprintf(buf, sizeof(buf), "%.0f KB/s", kbps);
    return buf;
}

static std::string fmt_eta(int secs) {
    char buf[64];
    if (secs <= 0) return "--:--";
    int m = secs / 60, s = secs % 60;
    if (m >= 60) {
        int h = m / 60; m %= 60;
        snprintf(buf, sizeof(buf), "%dh %02dm", h, m);
    } else {
        snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    }
    return buf;
}

static std::string fmt_bytes(uint64_t b) {
    char buf[64];
    if (b >= 1024ULL*1024*1024)
        snprintf(buf, sizeof(buf), "%.2f GB", b / (1024.0*1024*1024));
    else if (b >= 1024*1024)
        snprintf(buf, sizeof(buf), "%.1f MB", b / (1024.0*1024));
    else if (b >= 1024)
        snprintf(buf, sizeof(buf), "%.0f KB", b / 1024.0);
    else
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)b);
    return buf;
}

// ======================== UI ========================

UI::UI(Downloader& dl)
    : m_dl(dl)
    , m_state(AppState::MAIN_MENU)
    , m_prev_state(AppState::MAIN_MENU)
    , m_menu_sel(MenuSel::BTN_READY)
    , m_exit(false)
    , m_anim_t(0.f)
    , m_flash_timer(0)
{}

void UI::transition_to(AppState next) {
    m_prev_state = m_state;
    m_state      = next;
    m_anim_t     = 0.f;
}

void UI::update(uint32_t buttons, uint32_t prev_buttons) {
    m_anim_t    += 0.04f;
    m_flash_timer++;

    switch (m_state) {

    // ── القائمة الرئيسية ──────────────────────────────────
    case AppState::MAIN_MENU:
        if (just_pressed(PAD_DOWN, buttons, prev_buttons))
            m_menu_sel = MenuSel::BTN_SOON;
        if (just_pressed(PAD_UP, buttons, prev_buttons))
            m_menu_sel = MenuSel::BTN_READY;

        if (just_pressed(PAD_CROSS, buttons, prev_buttons)) {
            if (m_menu_sel == MenuSel::BTN_READY) {
                transition_to(AppState::PREPARING);
            } else {
                transition_to(AppState::COMING_SOON);
            }
        }
        if (just_pressed(PAD_OPTIONS, buttons, prev_buttons))
            transition_to(AppState::SETTINGS);
        break;

    // ── "Preparing your PS4..." ──────────────────────────
    case AppState::PREPARING:
        // بعد 90 إطار (~3 ثواني) نبدأ التحميل
        if ((int)(m_anim_t * 25) >= 90) {
            // بدء التحميل
            m_dl.start([this](const DownloadStatus& st) {
                m_status = st;
                if (st.finished) {
                    if (st.error && st.current_index == 0)
                        this->transition_to(AppState::ERROR_SCREEN);
                    else
                        this->transition_to(AppState::DONE);
                }
                if (st.error && !st.finished) {
                    // خطأ في ملف واحد – نكمل ولا نوقف
                }
            });
            transition_to(AppState::DOWNLOADING);
        }
        break;

    // ── شاشة التحميل ──────────────────────────────────────
    case AppState::DOWNLOADING:
        // التحديث يأتي عبر callback
        // الإلغاء بـ OPTIONS
        if (just_pressed(PAD_OPTIONS, buttons, prev_buttons)) {
            m_dl.cancel();
            transition_to(AppState::MAIN_MENU);
        }
        break;

    // ── انتهى التحميل ──────────────────────────────────────
    case AppState::DONE:
        if (just_pressed(PAD_CROSS, buttons, prev_buttons) ||
            just_pressed(PAD_CIRCLE, buttons, prev_buttons))
            transition_to(AppState::MAIN_MENU);
        break;

    // ── خطأ ───────────────────────────────────────────────
    case AppState::ERROR_SCREEN:
        if (just_pressed(PAD_CROSS, buttons, prev_buttons) ||
            just_pressed(PAD_CIRCLE, buttons, prev_buttons))
            transition_to(AppState::MAIN_MENU);
        break;

    // ── إعدادات ──────────────────────────────────────────
    case AppState::SETTINGS:
        if (just_pressed(PAD_CIRCLE, buttons, prev_buttons) ||
            just_pressed(PAD_OPTIONS, buttons, prev_buttons))
            transition_to(AppState::MAIN_MENU);
        break;

    // ── Coming Soon ───────────────────────────────────────
    case AppState::COMING_SOON:
        if ((int)(m_anim_t * 25) >= 120 ||
            just_pressed(PAD_CROSS,  buttons, prev_buttons) ||
            just_pressed(PAD_CIRCLE, buttons, prev_buttons))
            transition_to(AppState::MAIN_MENU);
        break;
    }
}

// ======================== رسم ========================

void UI::draw_background() {
    // خلفية تدريجية أسود → أزرق داكن
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = (float)y / SCREEN_HEIGHT;
        uint8_t r = (uint8_t)(0   * (1-t) + 0  * t);
        uint8_t g = (uint8_t)(0   * (1-t) + 10 * t);
        uint8_t b = (uint8_t)(15  * (1-t) + 40 * t);
        uint32_t color = 0xFF000000 | (b << 16) | (g << 8) | r;
        for (int x = 0; x < SCREEN_WIDTH; x++)
            draw_pixel(x, y, color);
    }

    // خطوط شبكة خفيفة (تأثير تقني)
    for (int y = 0; y < SCREEN_HEIGHT; y += 60)
        draw_line(0, y, SCREEN_WIDTH, y, 0xFF0A1A3A, 1);
    for (int x = 0; x < SCREEN_WIDTH; x += 80)
        draw_line(x, 0, x, SCREEN_HEIGHT, 0xFF0A1A3A, 1);

    // شريط علوي
    draw_rect(0, 0, SCREEN_WIDTH, 90, 0xFF000010);
    draw_line(0, 90, SCREEN_WIDTH, 90, COLOR_BLUE, 2);

    // شريط سفلي
    draw_rect(0, SCREEN_HEIGHT - 70, SCREEN_WIDTH, 70, 0xFF000010);
    draw_line(0, SCREEN_HEIGHT - 70, SCREEN_WIDTH, SCREEN_HEIGHT - 70, COLOR_BLUE_DARK, 1);
}

void UI::draw_header() {
    // شعار BilHubPS4 (كبير في الأعلى)
    draw_text_centered(20, "BilHubPS4", COLOR_BLUE_GLOW, 4);

    // نقطة توهّج تحت الشعار
    draw_line(760, 82, 1160, 82, COLOR_BLUE, 1);

    // أيقونة الإعدادات (أعلى اليمين)
    draw_text(SCREEN_WIDTH - 100, 28, "[OPT]", COLOR_GRAY, 2);
}

void UI::draw_button(int x, int y, int w, int h,
                     const char* label, bool selected, bool disabled) {
    uint32_t bg      = disabled ? 0xFF111122 : (selected ? COLOR_BLUE_DARK : 0xFF0D0D22);
    uint32_t border  = disabled ? 0xFF333355 : (selected ? COLOR_BLUE_GLOW : COLOR_BLUE_DARK);
    uint32_t txt_col = disabled ? COLOR_GRAY  : (selected ? COLOR_WHITE    : 0xFFAABBDD);
    int thick = selected ? 3 : 1;

    draw_rounded_rect(x, y, w, h, bg, 14);
    draw_rect_outline(x, y, w, h, 0, border, thick);

    // توهّج اختيار
    if (selected && !disabled) {
        draw_rounded_rect(x-2, y-2, w+4, h+4, 0x330066FF, 16);
    }

    // النص داخل الزر
    int tw = text_width(label, 3);
    draw_text((x + w/2) - tw/2, y + h/2 - 10, label, txt_col, 3);
}

void UI::draw_footer_hint(const char* hint) {
    draw_text_centered(SCREEN_HEIGHT - 48, hint, COLOR_GRAY, 2);
}

// ─── القائمة الرئيسية ───────────────────────────────────────────
void UI::draw_main_menu() {
    draw_background();
    draw_header();

    // زر 1: Let Your PS4 Ready For Use
    bool sel1 = (m_menu_sel == MenuSel::BTN_READY);
    draw_button(460, 220, 1000, 110, "Let Your PS4 Ready For Use", sel1);

    // زر 2: Soon (معطّل)
    bool sel2 = (m_menu_sel == MenuSel::BTN_SOON);
    draw_button(460, 370, 1000, 110, "Soon", sel2, true);

    // عدد الملفات
    char info[64];
    snprintf(info, sizeof(info), "%d files ready to download",
             (int)m_dl.get_files().size());
    draw_text_centered(530, info, 0xFF446688, 2);

    draw_footer_hint("X Select    UP/DOWN Navigate    OPTIONS Settings");
}

// ─── Preparing ──────────────────────────────────────────────────
void UI::draw_preparing() {
    draw_background();
    draw_header();

    // أنيميشن نقاط
    int dots = ((int)(m_anim_t * 8)) % 4;
    char msg[64] = "Preparing your PS4";
    for (int i = 0; i < dots; i++) strcat(msg, ".");

    draw_text_centered(440, msg, COLOR_BLUE_GLOW, 3);

    // شريط تحميل وهمي (تأثير فقط)
    float fake = (m_anim_t * 0.4f > 1.f) ? 1.f : m_anim_t * 0.4f;
    draw_progress_bar(360, 540, 1200, 24, fake,
                      COLOR_GRAY_DARK, COLOR_BLUE);

    draw_footer_hint("Please wait...");
}

// ─── التحميل ────────────────────────────────────────────────────
void UI::draw_downloading() {
    draw_background();
    draw_header();

    DownloadStatus st = m_dl.get_status();
    if (st.total_files == 0) st = m_status;

    // اسم الملف الحالي
    char file_label[128];
    snprintf(file_label, sizeof(file_label), "Downloading: %s",
             st.current_file.empty() ? "..." : st.current_file.c_str());
    draw_text_centered(160, file_label, COLOR_WHITE, 2);

    // رقم الملف من أصل الكل
    char count_label[64];
    snprintf(count_label, sizeof(count_label), "File %d of %d",
             st.current_index + 1, st.total_files);
    draw_text_centered(200, count_label, COLOR_GRAY, 2);

    // ─ شريط الملف الحالي
    draw_text(360, 290, "Current File:", COLOR_GRAY, 2);
    char pct_file[16];
    snprintf(pct_file, sizeof(pct_file), "%.1f%%", st.file_progress * 100.f);
    draw_text_right(SCREEN_WIDTH - 360, 290, pct_file, COLOR_BLUE_GLOW, 2);
    draw_progress_bar(360, 320, 1200, 28,
                      st.file_progress, COLOR_GRAY_DARK, COLOR_BLUE);

    // ─ شريط الإجمالي
    draw_text(360, 400, "Total Progress:", COLOR_GRAY, 2);
    char pct_total[16];
    snprintf(pct_total, sizeof(pct_total), "%.1f%%", st.total_progress * 100.f);
    draw_text_right(SCREEN_WIDTH - 360, 400, pct_total, COLOR_BLUE_GLOW, 2);
    draw_progress_bar(360, 430, 1200, 36,
                      st.total_progress, COLOR_GRAY_DARK, COLOR_BLUE_GLOW);

    // ─ معلومات إضافية
    // السرعة
    draw_text(360, 510, "Speed:", COLOR_GRAY, 2);
    std::string sp = fmt_speed(st.speed_kbps);
    draw_text(560, 510, sp.c_str(), COLOR_GREEN, 2);

    // الوقت المتبقي
    draw_text(760, 510, "ETA:", COLOR_GRAY, 2);
    std::string eta = fmt_eta(st.eta_seconds);
    draw_text(880, 510, eta.c_str(), COLOR_YELLOW, 2);

    // حجم محمّل
    draw_text(1060, 510, "Downloaded:", COLOR_GRAY, 2);
    std::string sz = fmt_bytes(st.downloaded_bytes);
    draw_text(1300, 510, sz.c_str(), COLOR_WHITE, 2);

    // خطأ في ملف واحد (ليس خطأ كامل)
    if (st.error && !st.finished) {
        char err[128];
        snprintf(err, sizeof(err), "Warning: %s", st.error_msg.c_str());
        draw_text_centered(570, err, COLOR_YELLOW, 2);
    }

    draw_footer_hint("OPTIONS Cancel");
}

// ─── انتهى التحميل ──────────────────────────────────────────────
void UI::draw_done() {
    draw_background();
    draw_header();

    draw_text_centered(380, "All files downloaded successfully.", COLOR_GREEN, 3);

    draw_text_centered(470, "Files saved to:", COLOR_GRAY, 2);
    draw_text_centered(510, "/data/BilHubPS4/downloads/", COLOR_BLUE_GLOW, 2);
    draw_text_centered(560, "Open Package Installer to install them.", COLOR_GRAY, 2);

    // وميض X للرجوع
    if ((m_flash_timer / 30) % 2 == 0)
        draw_text_centered(650, "Press X to return", COLOR_WHITE, 2);

    draw_footer_hint("X Return to Menu");
}

// ─── إعدادات ────────────────────────────────────────────────────
void UI::draw_settings() {
    draw_background();
    draw_header();

    draw_text_centered(160, "Settings", COLOR_WHITE, 3);
    draw_line(400, 210, 1520, 210, COLOR_BLUE_DARK, 1);

    // معلومات
    int lx = 500, ly = 250, ls = 28;
    struct { const char* k; const char* v; } rows[] = {
        {"App Name",   "BilHubPS4"},
        {"Version",    "1.0.0"},
        {"Developer",  "BilHub"},
        {"Language",   "Arabic / English"},
        {"Resolution", "1920x1080"},
        {"Save Path",  "/data/BilHubPS4/downloads/"},
    };

    for (auto& r : rows) {
        draw_text(lx, ly, r.k, COLOR_GRAY, 2);
        draw_text(lx + 400, ly, r.v, COLOR_WHITE, 2);
        draw_line(lx, ly + ls, 1400, ly + ls, 0xFF111133, 1);
        ly += ls + 10;
    }

    draw_footer_hint("CIRCLE / OPTIONS Back");
}

// ─── Coming Soon ────────────────────────────────────────────────
void UI::draw_coming_soon() {
    draw_background();
    draw_header();

    draw_text_centered(440, "Coming Soon...", COLOR_BLUE_GLOW, 4);
    draw_footer_hint("X / CIRCLE Back");
}

// ─── خطأ ────────────────────────────────────────────────────────
void UI::draw_error() {
    draw_background();
    draw_header();

    draw_text_centered(380, "Connection Error", COLOR_RED, 3);
    draw_text_centered(450, "Failed to download files.", COLOR_WHITE, 2);
    draw_text_centered(490, "Check your internet connection and try again.", COLOR_GRAY, 2);

    if ((m_flash_timer / 30) % 2 == 0)
        draw_text_centered(590, "Press X to return", COLOR_WHITE, 2);

    draw_footer_hint("X Return to Menu");
}

// ─── الرسم الرئيسي ──────────────────────────────────────────────
void UI::draw() {
    switch (m_state) {
    case AppState::MAIN_MENU:    draw_main_menu();   break;
    case AppState::PREPARING:    draw_preparing();   break;
    case AppState::DOWNLOADING:  draw_downloading(); break;
    case AppState::DONE:         draw_done();        break;
    case AppState::ERROR_SCREEN: draw_error();       break;
    case AppState::SETTINGS:     draw_settings();    break;
    case AppState::COMING_SOON:  draw_coming_soon(); break;
    }
}
