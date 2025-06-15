#ifndef COLLECTOR_HPP_
#define COLLECTOR_HPP_

#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "metric.hpp"

namespace metrics {

class MetricsCollector {
public:
    MetricsCollector() = default;

    void register_metric(std::shared_ptr<Metric> metric);
    void flush(const std::string &filename);

private:
    std::string current_timestamp();

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Metric>> metrics_;
};

}  // namespace metrics

#endif