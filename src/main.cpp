#include "renderer.h"
#include "downloader.h"
#include "ui.h"

#include <orbis/libkernel.h>
#include <orbis/SystemContent/Common/UserService.h>
#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ======================== مسار downloads.json ========================
// التطبيق يبحث أولاً في /data/BilHubPS4/downloads.json
// (يمكن تعديله عبر FTP بدون إعادة بناء الـ PKG)
// إذا لم يجده يستخدم النسخة المدمجة في التطبيق

static const char* EXTERNAL_JSON = "/data/BilHubPS4/downloads.json";
static const char* INTERNAL_JSON = "/app/downloads.json"; // داخل الـ PKG

// ======================== مدخل التطبيق ========================
int main() {
    // تهيئة خدمة المستخدم
    SceUserServiceInitializeParams us_params;
    memset(&us_params, 0, sizeof(us_params));
    us_params.priority = SCE_KERNEL_PRIO_FIFO_DEFAULT;
    sceUserServiceInitialize(&us_params);

    // الحصول على معرف المستخدم
    SceUserServiceUserId user_id = -1;
    sceUserServiceGetInitialUser(&user_id);

    // تهيئة Pad (يد التحكم)
    scePadInit();
    int pad_handle = scePadOpen(user_id, SCE_PAD_PORT_TYPE_STANDARD, 0, nullptr);

    // تهيئة نظام الرسم
    if (!renderer_init()) {
        sceSystemServiceLoadExec("exit", nullptr);
        return 1;
    }

    // إنشاء مجلدات الحفظ
    mkdir("/data", 0777);
    mkdir("/data/BilHubPS4", 0777);
    mkdir("/data/BilHubPS4/downloads", 0777);

    // ─── إعداد Downloader ───────────────────────────────────────
    Downloader dl;

    // حاول تحميل الـ JSON الخارجي أولاً (قابل للتعديل عبر FTP)
    bool loaded = false;
    if (!loaded) loaded = dl.load_list(EXTERNAL_JSON);
    if (!loaded) loaded = dl.load_list(INTERNAL_JSON);

    // إذا لم يجد أي ملف JSON، استخدم القائمة المدمجة مباشرةً
    if (!loaded) {
        // القائمة المدمجة - نفس محتوى downloads.json
        // (تُحدَّث هنا إذا أردت تضمينها في الكود)
        static const char EMBEDDED_JSON[] = R"([
  {"name":"FPKGi Data.pkg","url":"https://archive.org/download/app_20260723/FPKGi%20Data.pkg"},
  {"name":"FPKGi.pkg","url":"https://archive.org/download/app_20260723/FPKGi.pkg"},
  {"name":"PS4_APOL00004_v2.3.2.pkg","url":"https://archive.org/download/app_20260723/PS4_APOL00004_v2.3.2.pkg"},
  {"name":"PS4_CHTM00777_v1.2.2.pkg","url":"https://archive.org/download/app_20260723/PS4_CHTM00777_v1.2.2.pkg"},
  {"name":"PS4_CUSA00127_v1.53.pkg","url":"https://archive.org/download/app_20260723/PS4_CUSA00127_v1.53.pkg"},
  {"name":"PS4_ITEM00001_v1.08.pkg","url":"https://archive.org/download/app_20260723/PS4_ITEM00001_v1.08%20(1).pkg"},
  {"name":"PS4_LAPY20009_v2.07.pkg","url":"https://archive.org/download/app_20260723/PS4_LAPY20009_v2.07.pkg"},
  {"name":"app.pkg","url":"https://archive.org/download/app_20260723/app.pkg"}
])";
        // حفظ الـ JSON المدمج كملف خارجي للمرة الأولى
        FILE* fp = fopen(EXTERNAL_JSON, "w");
        if (fp) {
            fwrite(EMBEDDED_JSON, 1, sizeof(EMBEDDED_JSON)-1, fp);
            fclose(fp);
        }
        dl.load_list(EXTERNAL_JSON);
    }

    // ─── الواجهة ────────────────────────────────────────────────
    UI ui(dl);

    ScePadData pad_data;
    ScePadData pad_prev;
    memset(&pad_data, 0, sizeof(pad_data));
    memset(&pad_prev, 0, sizeof(pad_prev));

    // ═══════════════════════════════════════════
    // حلقة التطبيق الرئيسية
    // ═══════════════════════════════════════════
    while (!ui.should_exit()) {
        // قراءة مدخلات اليد
        pad_prev = pad_data;
        scePadReadState(pad_handle, &pad_data);

        uint32_t btns      = pad_data.buttons;
        uint32_t btns_prev = pad_prev.buttons;

        // تحديث المنطق
        ui.update(btns, btns_prev);

        // رسم الإطار الحالي
        renderer_clear(COLOR_BG);
        ui.draw();
        renderer_flip();

        // ~60fps
        sceKernelUsleep(16667);
    }

    // تنظيف
    renderer_shutdown();
    scePadClose(pad_handle);
    sceUserServiceTerminate();

    return 0;
}
