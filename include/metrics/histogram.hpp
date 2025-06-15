#ifndef HISTOGRAM_HPP_
#define HISTOGRAM_HPP_

#include "metric.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace metrics {

class Histogram : public Metric {
public:

    struct Snapshot {
        double sum;
        uint64_t count;
        std::vector<double> buckets;
        std::vector<uint64_t> counters;
    };

    Histogram(std::string name, const std::vector<double>& buckets);
    Histogram() = delete;

    void observe(double value);
    Snapshot get() const;

    std::string name() const override;
    std::string value_as_str() const override;
    void reset() override;

private:

    struct Inner {
        double sum = 0.0;
        uint64_t count = 0;
        std::vector<double> buckets;
        std::vector<uint64_t> counters;
    };

    std::string name_;
    std::shared_ptr<std::shared_mutex> mutex_;
    std::unique_ptr<Inner> inner_;
};

std::vector<double> exponential_buckets(double start, double factor, size_t length);
std::vector<double> linear_buckets(double start, double width, size_t length);
std::vector<double> exponential_buckets_range(double min, double max, size_t length);

} // namespace metrics

#endif