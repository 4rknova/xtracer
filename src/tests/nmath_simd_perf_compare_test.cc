#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <array>
#include <algorithm>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define XT_POPEN _popen
#define XT_PCLOSE _pclose
#else
#define XT_POPEN popen
#define XT_PCLOSE pclose
#endif

namespace {

double median_of(std::vector<double> values);
double p95_of(std::vector<double> values);

bool parse_total_ms_line(const std::string &line, double &out_ms)
{
    double ms = 0.0;
    if (std::sscanf(line.c_str(), "total%*[^0-9]%lf", &ms) == 1 && std::isfinite(ms) && ms > 0.0) {
        out_ms = ms;
        return true;
    }
    return false;
}

bool parse_metric_line(const std::string &line, std::string &out_key, double &out_ms)
{
    const std::string prefix = "metric|";
    if (line.compare(0, prefix.size(), prefix) != 0) return false;
    const std::size_t sep = line.find('|', prefix.size());
    if (sep == std::string::npos) return false;
    const std::string key = line.substr(prefix.size(), sep - prefix.size());
    if (key.empty()) return false;
    const std::string value_text = line.substr(sep + 1);
    const double ms = std::strtod(value_text.c_str(), nullptr);
    if (!std::isfinite(ms) || ms <= 0.0) return false;
    out_key = key;
    out_ms = ms;
    return true;
}

std::string parse_mode_line(const std::string &line)
{
    const char *prefix = "mode";
    if (line.compare(0, std::strlen(prefix), prefix) != 0) return "";
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) return "";
    std::string mode = line.substr(colon + 1);
    while (!mode.empty() && (mode.front() == ' ' || mode.front() == '\t')) mode.erase(mode.begin());
    while (!mode.empty() && (mode.back() == ' ' || mode.back() == '\t' || mode.back() == '\r' || mode.back() == '\n')) mode.pop_back();
    return mode;
}

bool run_bench_and_extract_metrics(
    const std::string &exe_path,
    uint64_t iters,
    std::map<std::string, double> &out_metrics,
    std::string &out_mode,
    std::string &out_text)
{
    std::ostringstream cmd;
    cmd << '"' << exe_path << '"' << " --iters " << iters;

    FILE *pipe = XT_POPEN(cmd.str().c_str(), "r");
    if (!pipe) {
        std::fprintf(stderr, "failed to spawn bench: %s (errno=%d)\n", exe_path.c_str(), errno);
        return false;
    }

    std::array<char, 512> buf{};
    out_text.clear();
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        out_text += buf.data();
    }

    const int rc = XT_PCLOSE(pipe);
    if (rc != 0) {
        std::fprintf(stderr, "bench returned non-zero exit code (%d): %s\n", rc, exe_path.c_str());
        return false;
    }

    std::istringstream lines(out_text);
    std::string line;
    out_mode.clear();
    out_metrics.clear();
    while (std::getline(lines, line)) {
        if (out_mode.empty()) {
            const std::string parsed_mode = parse_mode_line(line);
            if (!parsed_mode.empty()) out_mode = parsed_mode;
        }
        std::string metric_key;
        double metric_ms = 0.0;
        if (parse_metric_line(line, metric_key, metric_ms)) {
            out_metrics[metric_key] = metric_ms;
        }
    }

    if (out_metrics.empty()) {
        double total_ms = 0.0;
        std::istringstream legacy_lines(out_text);
        while (std::getline(legacy_lines, line)) {
            if (parse_total_ms_line(line, total_ms)) {
                out_metrics["total"] = total_ms;
                break;
            }
        }
    }

    if (out_metrics.find("total") == out_metrics.end()) {
        std::fprintf(stderr, "failed to parse benchmark metrics from output: %s\n", exe_path.c_str());
        return false;
    }
    if (out_mode.empty()) out_mode = "unknown";
    return true;
}

bool collect_required_metrics(
    const char *mode_label,
    const std::map<std::string, double> &metrics,
    std::map<std::string, std::vector<double>> &samples,
    const std::array<const char *, 7> &required_keys)
{
    for (std::size_t i = 0; i < required_keys.size(); ++i) {
        const char *key = required_keys[i];
        const auto it = metrics.find(key);
        if (it == metrics.end()) {
            std::fprintf(stderr, "%s bench missing metric: %s\n", mode_label, key);
            return false;
        }
        samples[key].push_back(it->second);
    }
    return true;
}

bool validate_metric_sample_counts(
    const char *mode_label,
    const std::map<std::string, std::vector<double>> &samples,
    const std::array<const char *, 7> &required_keys,
    std::size_t expected_count)
{
    for (std::size_t i = 0; i < required_keys.size(); ++i) {
        const char *key = required_keys[i];
        const auto it = samples.find(key);
        if (it == samples.end() || it->second.size() != expected_count) {
            std::fprintf(stderr, "%s metric sample count mismatch for %s (got=%zu expected=%zu)\n",
                mode_label,
                key,
                (it == samples.end() ? static_cast<std::size_t>(0) : it->second.size()),
                expected_count);
            return false;
        }
    }
    return true;
}

void print_metric_line(
    const char *name,
    const std::vector<double> &simd_samples,
    const std::vector<double> &avx_samples,
    const std::vector<double> &scalar_samples)
{
    const double simd_med = median_of(simd_samples);
    const double avx_med = median_of(avx_samples);
    const double scalar_med = median_of(scalar_samples);
    const double simd_p95 = p95_of(simd_samples);
    const double avx_p95 = p95_of(avx_samples);
    const double scalar_p95 = p95_of(scalar_samples);
    const double simd_vs_scalar = scalar_med / simd_med;
    const double avx_vs_scalar = scalar_med / avx_med;
    const double avx_vs_simd = simd_med / avx_med;
    std::printf("  | %-15s | %9.3f | %9.3f | %9.3f | %9.3f | %9.3f | %9.3f | %11.3f | %11.3f | %9.3f |\n",
        name, simd_med, simd_p95, avx_med, avx_p95, scalar_med, scalar_p95, simd_vs_scalar, avx_vs_scalar, avx_vs_simd);
}

double median_of(std::vector<double> values)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();
    if ((n % 2) == 1) return values[n / 2];
    return (values[(n / 2) - 1] + values[n / 2]) * 0.5;
}

double p95_of(std::vector<double> values)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();
    std::size_t idx = static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(n)));
    if (idx == 0) idx = 1;
    if (idx > n) idx = n;
    return values[idx - 1];
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s <simd_bench_path> <avx_bench_path> <scalar_bench_path> [--iters N]\n", argv[0]);
        return 2;
    }

    const std::string simd_path = argv[1];
    const std::string avx_path = argv[2];
    const std::string scalar_path = argv[3];
    uint64_t iters = 1200000ULL;
    unsigned int repeats = 9;

    for (int i = 4; i < argc; ++i) {
        if (std::strcmp(argv[i], "--iters") == 0 && (i + 1) < argc) {
            const unsigned long long parsed = std::strtoull(argv[i + 1], nullptr, 10);
            if (parsed > 0ULL) iters = static_cast<uint64_t>(parsed);
            i += 1;
        } else if (std::strcmp(argv[i], "--repeats") == 0 && (i + 1) < argc) {
            const unsigned long long parsed = std::strtoull(argv[i + 1], nullptr, 10);
            if (parsed > 0ULL) repeats = static_cast<unsigned int>(parsed);
            i += 1;
        }
    }

    const std::array<const char *, 7> required_metric_keys = {
        "prng_c",
        "vec3_normalize",
        "vec3_dot_cross",
        "mat4_mul",
        "vec3_batch_normalize",
        "vec3_batch_dot_cross",
        "total"
    };
    std::map<std::string, std::vector<double>> simd_samples;
    std::map<std::string, std::vector<double>> avx_samples;
    std::map<std::string, std::vector<double>> scalar_samples;
    for (std::size_t i = 0; i < required_metric_keys.size(); ++i) {
        simd_samples[required_metric_keys[i]].reserve(repeats);
        avx_samples[required_metric_keys[i]].reserve(repeats);
        scalar_samples[required_metric_keys[i]].reserve(repeats);
    }
    std::string simd_out;
    std::string avx_out;
    std::string scalar_out;
    std::string simd_mode_expected;
    std::string avx_mode_expected;
    std::string scalar_mode_expected;

    for (unsigned int rep = 0; rep < repeats; ++rep) {
        const int order = static_cast<int>(rep % 3);
        for (int step = 0; step < 3; ++step) {
            const int slot = (order + step) % 3;
            std::string mode;
            std::string out_text;
            std::map<std::string, double> metrics;
            bool ok = false;
            if (slot == 0) {
                ok = run_bench_and_extract_metrics(simd_path, iters, metrics, mode, out_text);
                if (!ok) return 1;
                if (simd_mode_expected.empty()) simd_mode_expected = mode;
                else if (simd_mode_expected != mode) {
                    std::fprintf(stderr, "simd mode changed across runs: expected=%s got=%s\n", simd_mode_expected.c_str(), mode.c_str());
                    return 1;
                }
                if (!collect_required_metrics("simd", metrics, simd_samples, required_metric_keys)) return 1;
            } else if (slot == 1) {
                ok = run_bench_and_extract_metrics(avx_path, iters, metrics, mode, out_text);
                if (!ok) return 1;
                if (avx_mode_expected.empty()) avx_mode_expected = mode;
                else if (avx_mode_expected != mode) {
                    std::fprintf(stderr, "avx mode changed across runs: expected=%s got=%s\n", avx_mode_expected.c_str(), mode.c_str());
                    return 1;
                }
                if (!collect_required_metrics("avx", metrics, avx_samples, required_metric_keys)) return 1;
            } else {
                ok = run_bench_and_extract_metrics(scalar_path, iters, metrics, mode, out_text);
                if (!ok) return 1;
                if (scalar_mode_expected.empty()) scalar_mode_expected = mode;
                else if (scalar_mode_expected != mode) {
                    std::fprintf(stderr, "scalar mode changed across runs: expected=%s got=%s\n", scalar_mode_expected.c_str(), mode.c_str());
                    return 1;
                }
                if (!collect_required_metrics("scalar", metrics, scalar_samples, required_metric_keys)) return 1;
            }
        }
    }

    if (!validate_metric_sample_counts("simd", simd_samples, required_metric_keys, repeats) ||
        !validate_metric_sample_counts("avx", avx_samples, required_metric_keys, repeats) ||
        !validate_metric_sample_counts("scalar", scalar_samples, required_metric_keys, repeats)) {
        std::fprintf(stderr, "no timing samples collected\n");
        return 1;
    }

    if (scalar_mode_expected != "scalar") {
        std::fprintf(stderr, "expected scalar bench mode=scalar, got: %s\n", scalar_mode_expected.c_str());
        return 1;
    }

    const std::vector<double> &simd_runs = simd_samples["total"];
    const std::vector<double> &avx_runs = avx_samples["total"];
    const std::vector<double> &scalar_runs = scalar_samples["total"];
    const double simd_med = median_of(simd_runs);
    const double avx_med = median_of(avx_runs);
    const double scalar_med = median_of(scalar_runs);
    const double simd_p95 = p95_of(simd_runs);
    const double avx_p95 = p95_of(avx_runs);

    const double speedup_simd_vs_scalar = scalar_med / simd_med;
    const double speedup_avx_vs_scalar = scalar_med / avx_med;
    const double speedup_avx_vs_simd = simd_med / avx_med;
    const double delta_simd_pct = ((scalar_med - simd_med) / scalar_med) * 100.0;
    const double delta_avx_pct = ((scalar_med - avx_med) / scalar_med) * 100.0;

    std::printf("nmath SIMD perf compare\n");
    std::printf("  iterations: %llu  repeats: %u  modes: simd=%s avx=%s scalar=%s\n",
        static_cast<unsigned long long>(iters), repeats, simd_mode_expected.c_str(), avx_mode_expected.c_str(), scalar_mode_expected.c_str());
    std::printf("  +-----------+-----------+-----------+-----------+-----------+-----------+\n");
    std::printf("  | Metric    | SIMD med  | SIMD p95  | AVX med   | AVX p95   | Scalar med|\n");
    std::printf("  +-----------+-----------+-----------+-----------+-----------+-----------+\n");
    std::printf("  | total ms  | %9.3f | %9.3f | %9.3f | %9.3f | %9.3f |\n", simd_med, simd_p95, avx_med, avx_p95, scalar_med);
    std::printf("  +-----------+-----------+-----------+-----------+-----------+-----------+\n");
    std::printf("  +----------------+-------------+-------------+-------------+\n");
    std::printf("  | Ratio/Delta    | Value       |             |             |\n");
    std::printf("  +----------------+-------------+-------------+-------------+\n");
    std::printf("  | simd/scalar (x)| %11.3f |             |             |\n", speedup_simd_vs_scalar);
    std::printf("  | avx/scalar (x) | %11.3f |             |             |\n", speedup_avx_vs_scalar);
    std::printf("  | avx/simd (x)   | %11.3f |             |             |\n", speedup_avx_vs_simd);
    std::printf("  | simd delta (%%) | %11.2f |             |             |\n", delta_simd_pct);
    std::printf("  | avx delta (%%)  | %11.2f |             |             |\n", delta_avx_pct);
    std::printf("  +----------------+-------------+-------------+-------------+\n");
    std::printf("  per-kernel table:\n");
    std::printf("  +-----------------+-----------+-----------+-----------+-----------+-----------+-----------+-------------+-------------+-----------+\n");
    std::printf("  | Kernel          | SIMD med  | SIMD p95  | AVX med   | AVX p95   | Scalar med| Scalar p95| simd/scalar | avx/scalar  | avx/simd  |\n");
    std::printf("  +-----------------+-----------+-----------+-----------+-----------+-----------+-----------+-------------+-------------+-----------+\n");
    print_metric_line("prng_c", simd_samples["prng_c"], avx_samples["prng_c"], scalar_samples["prng_c"]);
    print_metric_line("vec3_normalize", simd_samples["vec3_normalize"], avx_samples["vec3_normalize"], scalar_samples["vec3_normalize"]);
    print_metric_line("vec3_dot_cross", simd_samples["vec3_dot_cross"], avx_samples["vec3_dot_cross"], scalar_samples["vec3_dot_cross"]);
    print_metric_line("mat4_mul", simd_samples["mat4_mul"], avx_samples["mat4_mul"], scalar_samples["mat4_mul"]);
    print_metric_line("batch_normalize", simd_samples["vec3_batch_normalize"], avx_samples["vec3_batch_normalize"], scalar_samples["vec3_batch_normalize"]);
    print_metric_line("batch_dot_cross", simd_samples["vec3_batch_dot_cross"], avx_samples["vec3_batch_dot_cross"], scalar_samples["vec3_batch_dot_cross"]);
    std::printf("  +-----------------+-----------+-----------+-----------+-----------+-----------+-----------+-------------+-------------+-----------+\n");

    return 0;
}
