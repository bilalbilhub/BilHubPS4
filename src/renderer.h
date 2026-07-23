#pragma once
#include <stdint.h>

// رزولوشن PS4
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

// ألوان BilHubPS4
#define COLOR_BLACK      0xFF000000
#define COLOR_BG         0xFF0A0A0F
#define COLOR_BLUE       0xFF0066FF
#define COLOR_BLUE_DARK  0xFF003399
#define COLOR_BLUE_GLOW  0xFF2288FF
#define COLOR_WHITE      0xFFFFFFFF
#define COLOR_GRAY       0xFF888888
#define COLOR_GRAY_DARK  0xFF222233
#define COLOR_GREEN      0xFF00CC66
#define COLOR_RED        0xFFFF3333
#define COLOR_YELLOW     0xFFFFCC00

// مؤشر الخلفية (framebuffer)
extern uint32_t* g_framebuffer;

// تهيئة نظام الرسم
bool renderer_init();
void renderer_shutdown();

// مسح الشاشة
void renderer_clear(uint32_t color = COLOR_BG);

// رسم شكل مستطيل
void draw_rect(int x, int y, int w, int h, uint32_t color);

// رسم مستطيل بإطار
void draw_rect_outline(int x, int y, int w, int h, uint32_t fill, uint32_t border, int border_thickness = 2);

// رسم مستطيل بزوايا دائرية (محاكاة)
void draw_rounded_rect(int x, int y, int w, int h, uint32_t color, int radius = 12);

// رسم نص (ASCII بسيط مدمج)
void draw_text(int x, int y, const char* text, uint32_t color, int scale = 1);
void draw_text_centered(int y, const char* text, uint32_t color, int scale = 1);
void draw_text_right(int x, int y, const char* text, uint32_t color, int scale = 1);

// رسم شريط التقدم
void draw_progress_bar(int x, int y, int w, int h, float percent, uint32_t bg_color, uint32_t fill_color);

// رسم الخط
void draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness = 1);

// رسم بكسل
inline void draw_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT)
        g_framebuffer[y * SCREEN_WIDTH + x] = color;
}

// عرض الإطار
void renderer_flip();

// دالة مساعدة للحصول على عرض النص
int text_width(const char* text, int scale = 1);
