#ifndef XTRACER_FRONTEND_WEB_REQUEST_LOGGER_MIDDLEWARE_H_INCLUDED
#define XTRACER_FRONTEND_WEB_REQUEST_LOGGER_MIDDLEWARE_H_INCLUDED

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include <crow_all.h>

// Set to true by main() when --verbose is passed.
extern bool g_web_verbose;

namespace {

static inline bool rlm_stdout_supports_ansi()
{
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    if (isatty(fileno(stdout)) == 0) return false;
    const char *term = std::getenv("TERM");
    if (!term || std::strcmp(term, "dumb") == 0) return false;
    return true;
#endif
}

static inline const char *rlm_http_status_color(int status)
{
    if (status >= 200 && status < 300) return "\033[32m";
    if (status >= 300 && status < 400) return "\033[36m";
    if (status >= 400 && status < 500) return "\033[33m";
    if (status >= 500 && status < 600) return "\033[31m";
    return "\033[37m";
}

static inline bool rlm_is_safe_client_id_char(unsigned char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_' || c == '-' || c == '.';
}

static inline std::string rlm_request_client_tag(const crow::request &req)
{
    auto parse_cookie = [&req]() -> std::string {
        const std::string cookie = req.get_header_value("Cookie");
        if (cookie.empty()) return "";
        const char *needle = "client_id=";
        const size_t n = std::strlen(needle);
        size_t i = 0;
        while (i < cookie.size()) {
            while (i < cookie.size() && (cookie[i] == ' ' || cookie[i] == ';')) ++i;
            if (i >= cookie.size()) break;
            if (cookie.compare(i, n, needle) == 0) {
                size_t begin = i + n;
                size_t end = begin;
                while (end < cookie.size() && cookie[end] != ';') ++end;
                return cookie.substr(begin, end - begin);
            }
            while (i < cookie.size() && cookie[i] != ';') ++i;
        }
        return "";
    };

    std::string client_id;
    const char *cid_param = req.url_params.get("client_id");
    if (cid_param) client_id = cid_param;
    if (client_id.empty()) client_id = parse_cookie();
    if (client_id.empty()) return "no-client";
    if (client_id.size() > 96) return "invalid-client";
    for (size_t i = 0; i < client_id.size(); ++i) {
        if (!rlm_is_safe_client_id_char((unsigned char)client_id[i])) return "invalid-client";
    }
    return client_id;
}

static inline std::string rlm_compact_client_tag(const std::string &tag)
{
    if (tag.size() > 7 && tag.compare(0, 7, "client_") == 0) return tag.substr(7);
    if (tag.size() > 7 && tag.compare(0, 7, "client-") == 0) return tag.substr(7);
    return tag;
}

static inline std::string rlm_cancel_job_annotation(const crow::request &req)
{
    if (req.method != crow::HTTPMethod::Post) return "";
    const std::string prefix = "/api/jobs/abort/";
    if (req.url.size() <= prefix.size()) return "";
    if (req.url.compare(0, prefix.size(), prefix) != 0) return "";
    const std::string id = req.url.substr(prefix.size());
    if (id.empty()) return "";
    return " cancel_job=" + id;
}

static inline int rlm_terminal_width_columns()
{
#if defined(_WIN32)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE) return 0;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(h, &info)) return 0;
    const int w = (int)(info.srWindow.Right - info.srWindow.Left + 1);
    return (w > 0) ? w : 0;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0) return 0;
    const int w = (int)ws.ws_col;
    return (w > 0) ? w : 0;
#endif
}

static inline std::string rlm_pad_right(const std::string &s, size_t width)
{
    if (s.size() >= width) return s;
    return s + std::string(width - s.size(), ' ');
}

static inline std::string rlm_pad_left(const std::string &s, size_t width)
{
    if (s.size() >= width) return s;
    return std::string(width - s.size(), ' ') + s;
}

static inline std::string rlm_fit_field(const std::string &s, size_t width)
{
    if (s.size() == width) return s;
    if (s.size() < width) return rlm_pad_right(s, width);
    if (width <= 3) return s.substr(0, width);
    return s.substr(0, width - 3) + "...";
}

static inline std::string rlm_now_timestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm tm_now;
#if defined(_WIN32)
    localtime_s(&tm_now, &t);
#else
    localtime_r(&t, &tm_now);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_now);
    return buf;
}

// Decode a WebSocket close reason into a human-readable string.
// WebSocket close frames carry an optional 2-byte big-endian status code
// followed by optional UTF-8 reason text.  Crow passes the raw bytes as-is,
// which renders as garbled output when printed.  This function extracts the
// numeric code and sanitises the text so the log stays readable.
static inline std::string rlm_ws_close_reason(const std::string &reason)
{
    if (reason.empty()) return "";
    if (reason.size() >= 2) {
        const uint16_t code = (static_cast<uint8_t>(reason[0]) << 8)
                            | static_cast<uint8_t>(reason[1]);
        if (code >= 1000 && code <= 4999) {
            std::string out = "code=" + std::to_string(code);
            if (reason.size() > 2) {
                out += " ";
                for (size_t i = 2; i < reason.size(); ++i) {
                    unsigned char c = static_cast<unsigned char>(reason[i]);
                    out += (c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : '?';
                }
            }
            return out;
        }
    }
    // No valid status code prefix — sanitise whatever is there.
    std::string out;
    out.reserve(reason.size());
    for (unsigned char c : reason)
        out += (c >= 0x20 && c < 0x7F) ? static_cast<char>(c) : '?';
    return out;
}

// Log a WebSocket lifecycle event in a format consistent with HTTP request logs.
// event: short uppercase label, e.g. "OPEN", "CLOSE", "ABORT"
// path:  full URL path including any relevant query params, e.g. "/ws/jobs/abc"
// detail: optional trailing annotation, e.g. "17 tiles catchup"
static inline void rlm_ws_log(
    const std::string &client_tag,
    const std::string &event,
    const std::string &path,
    const std::string &detail = "")
{
    if (!g_web_verbose) return;

    static const bool use_ansi = rlm_stdout_supports_ansi();

    const std::string ts           = rlm_now_timestamp();
    const std::string client_field = rlm_fit_field(rlm_compact_client_tag(client_tag), 18);
    const std::string event_field  = rlm_pad_right(event, 6);

    // Compose the detail annotation (space-separated, only when non-empty).
    const std::string annotation = detail.empty() ? "" : "  " + detail;

    if (use_ansi) {
        // Cyan " WS" to distinguish from HTTP status codes; event label in plain white.
        std::printf("[%s] [%s] \033[36m WS\033[0m %-6s %s%s\n",
                    ts.c_str(), client_field.c_str(),
                    event_field.c_str(),
                    path.c_str(),
                    annotation.c_str());
    } else {
        std::printf("[%s] [%s]  WS %-6s %s%s\n",
                    ts.c_str(), client_field.c_str(),
                    event_field.c_str(),
                    path.c_str(),
                    annotation.c_str());
    }
    std::fflush(stdout);
}

} // anonymous namespace

struct RequestLoggerMiddleware
{
    struct context
    {
        std::chrono::steady_clock::time_point start;
    };

    void before_handle(crow::request & /*req*/, crow::response & /*res*/, context &ctx)
    {
        ctx.start = std::chrono::steady_clock::now();
    }

    void after_handle(crow::request &req, crow::response &res, context &ctx)
    {
        if (!g_web_verbose) return;

        const long elapsed_ms = static_cast<long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - ctx.start).count());

        static const bool use_ansi = rlm_stdout_supports_ansi();

        const bool slow       = (elapsed_ms >= 500);
        const char *color     = slow ? "\033[31m" : rlm_http_status_color(res.code);
        const char *dur_color = slow ? "\033[31m" : "\033[90m";
        const char *reset     = "\033[0m";

        const std::string ts           = rlm_now_timestamp();
        const std::string client_tag   = rlm_compact_client_tag(rlm_request_client_tag(req));
        const std::string client_field = rlm_fit_field(client_tag, 18);
        const std::string method_str   = crow::method_name(req.method);
        const std::string method_field = rlm_pad_right(method_str, 6);
        const std::string cancel_note  = rlm_cancel_job_annotation(req);
        const std::string prefix_plain = "[" + ts + "] [" + client_field + "] "
            + std::to_string(res.code) + " "
            + method_field + " "
            + req.url
            + cancel_note;
        const std::string duration_plain = std::to_string(elapsed_ms) + "ms";
        const std::string duration_field = rlm_pad_left(duration_plain, 8);

        const int cols = rlm_terminal_width_columns();
        size_t pad_spaces = 1;
        if (cols > 0) {
            const size_t used = prefix_plain.size() + duration_field.size();
            if (static_cast<size_t>(cols) > used + 1)
                pad_spaces = static_cast<size_t>(cols) - used;
        }
        const std::string gap(pad_spaces, ' ');

        if (use_ansi) {
            std::printf("[%s] [%s] %s%3d%s %-6s %s%s%s%s%s\n",
                        ts.c_str(), client_field.c_str(),
                        color, res.code, reset,
                        method_str.c_str(),
                        (req.url + cancel_note).c_str(),
                        gap.c_str(), dur_color, duration_field.c_str(), reset);
        } else {
            std::printf("[%s] [%s] %3d %-6s %s%s%s\n",
                        ts.c_str(), client_field.c_str(),
                        res.code, method_str.c_str(),
                        (req.url + cancel_note).c_str(),
                        gap.c_str(), duration_field.c_str());
        }
        std::fflush(stdout);
    }
};

using WebApp = crow::App<RequestLoggerMiddleware>;

#endif /* XTRACER_FRONTEND_WEB_REQUEST_LOGGER_MIDDLEWARE_H_INCLUDED */
