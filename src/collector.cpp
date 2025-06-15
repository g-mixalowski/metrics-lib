#include "collector.hpp"
#include "metric.hpp"
#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <iostream>
    
void metrics::MetricsCollector::register_metric(std::shared_ptr<Metric> metric) {
    std::unique_lock lock(mutex_);
    metrics_.push_back(metric);
}

void metrics::MetricsCollector::flush(const std::string& filename) {
    std::vector<std::pair<std::string, std::string>> metrics_snapshot;
    metrics_snapshot.reserve(metrics_.size());
    {
        std::unique_lock lock(mutex_);
        for (auto &metric : metrics_) {
            metrics_snapshot.emplace_back(metric->name(), metric->value_as_str());
            metric->reset();
        }
    }

    std::ofstream file(filename, std::ios_base::app);
    if (!file) return;

    file << current_timestamp();
    for (auto& [name, value] : metrics_snapshot) {
        file << " \"" << name << "\" " << value;
    }
    file << std::endl;
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
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << now_ms.count();
    return oss.str();
}