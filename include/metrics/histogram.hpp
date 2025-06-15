#ifndef HISTOGRAM_HPP_
#define HISTOGRAM_HPP_

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "metric.hpp"

namespace metrics {

class Histogram : public Metric {
public:
    struct Snapshot {
        double sum;
        uint64_t count;
        std::vector<double> buckets;
        std::vector<uint64_t> counters;
    };

    template <
        typename S,
        typename B,
        typename = std::enable_if<std::is_convertible_v<S, std::string>>,
        typename =
            std::enable_if<std::is_convertible_v<B, std::vector<double>>>>
    Histogram(S &&name, B &&buckets)
        : name_(std::forward<S>(name)),
          inner_(std::make_unique<Inner>()),
          is_cached(false) {
        inner_->buckets = std::forward<B>(buckets);
        std::sort(inner_->buckets.begin(), inner_->buckets.end());
        inner_->buckets.push_back(std::numeric_limits<double>::infinity());
        inner_->counters.resize(inner_->buckets.size(), 0);
    }

    Histogram() = delete;
    Histogram(const Histogram &) = delete;
    Histogram(Histogram &&) = delete;
    Histogram &operator=(const Histogram &) = delete;
    Histogram &operator=(Histogram &&) = delete;

    void observe(double value) noexcept;
    Snapshot get() const noexcept;

    std::string_view name() const noexcept override;
    std::string value_as_str() const override;
    void reset() noexcept override;

private:
    struct Inner {
        double sum = 0.0;
        uint64_t count = 0;
        std::vector<double> buckets;
        std::vector<uint64_t> counters;
    };

    const std::string name_;
    std::unique_ptr<Inner> inner_;
    mutable bool is_cached;
    mutable std::string cached_value;
    mutable std::shared_mutex mutex_;
};

std::vector<double>
exponential_buckets(double start, double factor, size_t length);
std::vector<double> linear_buckets(double start, double width, size_t length);
std::vector<double>
exponential_buckets_range(double min, double max, size_t length);

}  // namespace metrics

#endif