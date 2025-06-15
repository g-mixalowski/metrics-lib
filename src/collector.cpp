#include "collector.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "metric.hpp"

void metrics::MetricsCollector::register_metric(std::shared_ptr<Metric> metric
) {
    std::unique_lock lock(mutex_);
    metrics_.push_back(metric);
}

void metrics::MetricsCollector::flush(const std::string &filename) {
    std::vector<std::pair<std::string, std::string>> metrics_snapshot;
    metrics_snapshot.reserve(metrics_.size());
    {
        std::unique_lock lock(mutex_);
        for (auto &metric : metrics_) {
            metrics_snapshot.emplace_back(
                metric->name(), metric->value_as_str()
            );
            metric->reset();
        }
    }

    size_t approx_length = 256;
    for (const auto &[name, value] : metrics_snapshot) {
        approx_length += name.size() + value.size() + 32;
    }

    std::string buffer;
    buffer.reserve(approx_length);
    buffer += current_timestamp();
    for (auto &[name, value] : metrics_snapshot) {
        buffer += " \"";
        buffer += name;
        buffer += "\" ";
        buffer += value;
    }
    buffer += '\n';

    FILE *file = fopen(filename.c_str(), "a");
    if (!file) {
        return;
    }
    fwrite(buffer.data(), 1, buffer.size(), file);
    fclose(file);
}

std::string metrics::MetricsCollector::current_timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_time_t = system_clock::to_time_t(now);
    auto now_ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm now_tm;
#ifdef _WIN32
    localtime_s(&now_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &now_tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "."
        << std::setfill('0') << std::setw(3) << now_ms.count();
    return std::move(oss).str();
}