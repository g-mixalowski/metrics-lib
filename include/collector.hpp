#ifndef COLLECTOR_HPP_
#define COLLECTOR_HPP_

#include <condition_variable>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include "metric.hpp"

namespace metrics {

class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector();

    void register_metric(std::shared_ptr<Metric> metric);
    void flush(std::string filename);

private:

    std::string current_timestamp();
    void write();
    void write_from_queue();

    mutable std::mutex mutex_;
    std::vector<std::shared_ptr<Metric>> metrics_;

    struct Task {
        std::string filename;
        std::string output;
    };

    std::thread writer_;
    std::queue<Task> writer_queue_;
    mutable std::mutex file_mutex_;
    std::condition_variable cv_;
    bool stopped_;
};

}  // namespace metrics

#endif