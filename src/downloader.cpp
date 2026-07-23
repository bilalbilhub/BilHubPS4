#include "downloader.h"
#include <orbis/libkernel.h>
#include <orbis/SystemContent/Common/UserService.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// مسار حفظ الملفات على PS4
const char* Downloader::SAVE_PATH = "/data/BilHubPS4/downloads";

// ======== هياكل مساعدة للـ HTTP ========
struct WriteData {
    FILE*    fp;
    uint64_t written;
};

struct ProgressData {
    Downloader*      self;
    int              index;
    int              total;
    ProgressCallback cb;
    uint64_t         start_time;
    uint64_t         last_bytes;
    double           speed;
};

// ======== دوال HTTP عبر libSceNet / libSceHttp ========
// نستخدم واجهة بسيطة عبر SceHttp

static int g_http_ctx  = -1;
static int g_ssl_ctx   = -1;
static int g_net_pool  = -1;

static bool http_init() {
    if (sceSysmoduleLoadModule(SCE_SYSMODULE_NET) < 0)       return false;
    if (sceSysmoduleLoadModule(SCE_SYSMODULE_SSL) < 0)       return false;
    if (sceSysmoduleLoadModule(SCE_SYSMODULE_HTTP) < 0)      return false;

    SceNetInitParam netParam;
    g_net_pool = sceNetPoolCreate("BilHubNet", 4 * 1024 * 1024, 0);
    if (g_net_pool < 0) return false;

    sceNetInit();

    g_ssl_ctx = sceSslInit(1024 * 1024);
    if (g_ssl_ctx < 0) return false;

    g_http_ctx = sceHttpInit(g_net_pool, g_ssl_ctx, 1024 * 1024);
    if (g_http_ctx < 0) return false;

    return true;
}

static void http_shutdown() {
    if (g_http_ctx >= 0) { sceHttpTerm(g_http_ctx); g_http_ctx = -1; }
    if (g_ssl_ctx  >= 0) { sceSslTerm(g_ssl_ctx);   g_ssl_ctx  = -1; }
    if (g_net_pool >= 0) { sceNetPoolDestroy(g_net_pool); g_net_pool = -1; }
    sceNetTerm();
}

// تحميل URL إلى ملف مع callback للتقدم
static bool http_download(const char* url, const char* dest_path,
                          ProgressData* prog) {
    int tmpl = sceHttpCreateTemplate(g_http_ctx, "BilHubPS4/1.0", SCE_HTTP_VERSION_1_1, 1);
    if (tmpl < 0) return false;

    int conn = sceHttpCreateConnectionWithURL(tmpl, url, 1);
    if (conn < 0) { sceHttpDeleteTemplate(tmpl); return false; }

    int req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, url, 0);
    if (req < 0) { sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tmpl); return false; }

    if (sceHttpSendRequest(req, nullptr, 0) < 0) {
        sceHttpDeleteRequest(req);
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tmpl);
        return false;
    }

    // حجم الملف
    uint64_t content_len = 0;
    sceHttpGetResponseContentLength(req, nullptr, &content_len);

    FILE* fp = fopen(dest_path, "wb");
    if (!fp) {
        sceHttpDeleteRequest(req);
        sceHttpDeleteConnection(conn);
        sceHttpDeleteTemplate(tmpl);
        return false;
    }

    uint8_t  buf[64 * 1024];
    uint64_t downloaded = 0;
    int      ret;
    uint64_t last_time  = sceKernelGetProcessTime();
    uint64_t last_bytes = 0;
    double   speed      = 0.0;

    while ((ret = sceHttpReadData(req, buf, sizeof(buf))) > 0) {
        if (prog->self->is_running() == false) {
            // إلغاء
            fclose(fp);
            remove(dest_path);
            sceHttpDeleteRequest(req);
            sceHttpDeleteConnection(conn);
            sceHttpDeleteTemplate(tmpl);
            return false;
        }

        fwrite(buf, 1, ret, fp);
        downloaded += ret;

        // حساب السرعة كل 500ms
        uint64_t now   = sceKernelGetProcessTime(); // microseconds
        uint64_t elapsed = now - last_time;
        if (elapsed >= 500000ULL) {
            double secs = elapsed / 1000000.0;
            speed = (downloaded - last_bytes) / secs / 1024.0;
            last_time  = now;
            last_bytes = downloaded;
        }

        // تحديث الحالة
        DownloadStatus st;
        st.current_file     = prog->self->get_files()[prog->index].name;
        st.current_index    = prog->index;
        st.total_files      = prog->total;
        st.file_progress    = content_len > 0 ? (float)downloaded / content_len : 0.f;
        st.total_progress   = ((float)prog->index + st.file_progress) / prog->total;
        st.speed_kbps       = speed;
        st.downloaded_bytes = downloaded;
        st.total_bytes      = content_len;
        st.eta_seconds      = (speed > 0 && content_len > downloaded)
                              ? (int)((content_len - downloaded) / 1024.0 / speed)
                              : 0;
        st.finished         = false;
        st.error            = false;

        if (prog->cb) prog->cb(st);
    }

    fclose(fp);
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tmpl);

    return (ret == 0); // 0 = نجح، سالب = خطأ
}

// ======== Downloader Implementation ========

Downloader::Downloader()
    : m_running(false), m_cancel(false) {}

Downloader::~Downloader() {
    cancel();
}

// parse JSON بسيط يدعم مصفوفة [{name,url}]
bool Downloader::parse_json(const std::string& json) {
    m_files.clear();
    size_t pos = 0;

    auto skip_ws = [&]() {
        while (pos < json.size() && (json[pos]==' '||json[pos]=='\t'||json[pos]=='\n'||json[pos]=='\r'))
            pos++;
    };

    auto read_string = [&]() -> std::string {
        if (pos >= json.size() || json[pos] != '"') return "";
        pos++; // skip "
        std::string out;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos+1 < json.size()) {
                pos++;
                if (json[pos] == 'n')       out += '\n';
                else if (json[pos] == 't')  out += '\t';
                else if (json[pos] == '"')  out += '"';
                else if (json[pos] == '\\') out += '\\';
                else out += json[pos];
            } else {
                out += json[pos];
            }
            pos++;
        }
        if (pos < json.size()) pos++; // skip closing "
        return out;
    };

    skip_ws(pos);
    if (pos >= json.size() || json[pos] != '[') return false;
    pos++; // skip [

    while (true) {
        skip_ws(pos);
        if (pos >= json.size() || json[pos] == ']') break;
        if (json[pos] == ',') { pos++; continue; }
        if (json[pos] != '{') { pos++; continue; }
        pos++; // skip {

        DownloadFile df;
        while (true) {
            skip_ws(pos);
            if (pos >= json.size() || json[pos] == '}') break;
            if (json[pos] == ',') { pos++; continue; }

            std::string key = read_string();
            skip_ws(pos);
            if (pos < json.size() && json[pos] == ':') pos++;
            skip_ws(pos);
            std::string val = read_string();

            if (key == "name") df.name = val;
            else if (key == "url") df.url = val;
            // تجاهل size وغيره
        }
        if (pos < json.size()) pos++; // skip }

        if (!df.name.empty() && !df.url.empty())
            m_files.push_back(df);
    }

    return !m_files.empty();
}

bool Downloader::load_list(const char* json_path) {
    FILE* fp = fopen(json_path, "r");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string content(sz, '\0');
    fread(&content[0], 1, sz, fp);
    fclose(fp);

    return parse_json(content);
}

bool Downloader::load_list_from_url(const char* url) {
    // يُحمّل downloads.json من URL ثم يقرأه
    const char* tmp = "/data/BilHubPS4/downloads.json";

    // تأكد المجلد موجود
    mkdir("/data/BilHubPS4", 0777);

    // تحميل بسيط بدون progress
    ProgressData pd;
    pd.self  = this;
    pd.index = 0;
    pd.total = 1;
    pd.cb    = nullptr;
    pd.speed = 0;

    if (!http_download(url, tmp, &pd)) return false;
    return load_list(tmp);
}

DownloadStatus Downloader::get_status() {
    return m_status;
}

void Downloader::cancel() {
    m_cancel  = true;
    m_running = false;
}

// Thread entry (سيُشغّل في ScePthread)
struct ThreadArgs {
    Downloader*      dl;
    ProgressCallback cb;
};

static void* download_thread(void* arg) {
    ThreadArgs* ta = (ThreadArgs*)arg;
    Downloader* dl = ta->dl;
    ProgressCallback cb = ta->cb;
    delete ta;

    // تهيئة HTTP
    if (!http_init()) {
        DownloadStatus st;
        st.error     = true;
        st.error_msg = "Failed to init network";
        st.finished  = true;
        if (cb) cb(st);
        dl->m_running = false;
        return nullptr;
    }

    // إنشاء مجلد الحفظ
    mkdir("/data/BilHubPS4", 0777);
    mkdir(Downloader::SAVE_PATH, 0777);

    const auto& files = dl->get_files();
    int total = (int)files.size();
    bool all_ok = true;

    for (int i = 0; i < total; i++) {
        if (dl->m_cancel) { all_ok = false; break; }

        std::string dest = std::string(Downloader::SAVE_PATH) + "/" + files[i].name;

        ProgressData pd;
        pd.self    = dl;
        pd.index   = i;
        pd.total   = total;
        pd.cb      = cb;
        pd.speed   = 0;
        pd.last_bytes = 0;
        pd.start_time = sceKernelGetProcessTime();

        bool ok = http_download(files[i].url.c_str(), dest.c_str(), &pd);

        if (!ok && !dl->m_cancel) {
            DownloadStatus st;
            st.current_file  = files[i].name;
            st.current_index = i;
            st.total_files   = total;
            st.error         = true;
            st.error_msg     = "Download failed: " + files[i].name;
            st.finished      = false;
            if (cb) cb(st);
            all_ok = false;
            // نكمل بدل ما نوقف
        }
    }

    http_shutdown();

    // انتهاء
    DownloadStatus st;
    st.finished       = true;
    st.error          = false;
    st.total_files    = total;
    st.total_progress = 1.0f;
    st.current_index  = total;
    if (!all_ok && !dl->m_cancel)
        st.error_msg = "Some files failed to download";
    if (cb) cb(st);

    dl->m_running = false;
    return nullptr;
}

void Downloader::start(ProgressCallback callback) {
    if (m_running) return;
    m_running = true;
    m_cancel  = false;

    ThreadArgs* args = new ThreadArgs;
    args->dl = this;
    args->cb = callback;

    ScePthread thread;
    ScePthreadAttr attr;
    scePthreadAttrInit(&attr);
    scePthreadAttrSetstacksize(&attr, 256 * 1024);
    scePthreadCreate(&thread, &attr, download_thread, args, "BilHubDownload");
    scePthreadAttrDestroy(&attr);
    scePthreadDetach(thread);
}
