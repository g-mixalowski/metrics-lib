#include "collector.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "metric.hpp"

metrics::MetricsCollector::MetricsCollector() {
    writer_ = std::thread(&MetricsCollector::write_from_queue, this);
}

metrics::MetricsCollector::~MetricsCollector() {
    {
        std::unique_lock lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_one();
    if (writer_.joinable()) {
        writer_.join();
    }
}

void metrics::MetricsCollector::register_metric(std::shared_ptr<Metric> metric
) {
    std::unique_lock lock(mutex_);
    metrics_.push_back(metric);
}

void metrics::MetricsCollector::flush(std::string filename) {
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

    {
        std::unique_lock lock(mutex_);
        writer_queue_.emplace(std::move(filename), std::move(buffer));
    }
    cv_.notify_one();
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

void metrics::MetricsCollector::write_from_queue() {
    while(!stopped_) {
        std::string filename;
        std::string buffer;
        {
            std::unique_lock lock(file_mutex_);
            cv_.wait(lock, [this] {
                return !writer_queue_.empty() || stopped_;
            });

            if (!writer_queue_.empty()) {
                filename = std::move(writer_queue_.front().filename);
                buffer = std::move(writer_queue_.front().output);
                writer_queue_.pop();
            }
        }

        if (!buffer.empty()) {
            FILE* file = fopen(filename.c_str(), "a");
            if (!file) {
                continue;
            }
            fwrite(buffer.data(), 1, buffer.size(), file);
            fclose(file);
        }
    }
}