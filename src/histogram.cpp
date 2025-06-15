#include "histogram.hpp"
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

metrics::Histogram::Histogram(std::string name, const std::vector<double>& buckets) {
    auto inner = std::make_unique<Inner>();
    inner->buckets = buckets;
    std::sort(inner->buckets.begin(), inner->buckets.end());
    inner->buckets.push_back(std::numeric_limits<double>::infinity());
    inner->counters.resize(inner->buckets.size(), 0);
    
    name_ = name;
    mutex_ = std::make_shared<std::shared_mutex>();
    inner_ = std::move(inner);
}

void metrics::Histogram::observe(double value) {
    std::unique_lock lock(*mutex_);
    auto& data = *inner_;
    data.sum += value;
    data.count++;
    
    auto it = std::lower_bound(data.buckets.begin(), data.buckets.end(), value);
    if (it != data.buckets.end()) {
        size_t index = std::distance(data.buckets.begin(), it);
        data.counters[index]++;
    }
}

metrics::Histogram::Snapshot metrics::Histogram::get() const {
    std::shared_lock lock(*mutex_);
    return {
        inner_->sum,
        inner_->count,
        inner_->buckets,
        inner_->counters
    };
}


std::string metrics::Histogram::name() const {
    return name_;
}

std::string metrics::Histogram::value_as_str() const {
    Snapshot snapshot = get();
    uint64_t sum = 0;

    std::ostringstream oss;
    oss << "{";
    for (std::size_t i = 0; i < snapshot.buckets.size(); ++i) {
        sum += snapshot.counters[i];
        oss << "\"" << name_ << "_bucket{le=";
        if (std::isinf(snapshot.buckets[i])) oss << "+Inf";
        else oss << snapshot.buckets[i];
        oss << "}\" " << sum << " ";
    }
    oss << " \"" << name_ << "_sum\" " << snapshot.sum << " ";
    oss << " \"" << name_ << "_count\" " << snapshot.count << "}";

    return oss.str();
}

void metrics::Histogram::reset() {
    for (auto &counter : inner_->counters) counter = 0;
    inner_->sum = 0.0;
    inner_->count = 0;
}

std::vector<double> metrics::exponential_buckets(double start, double factor, size_t length) {
    std::vector<double> buckets;
    double current = start;
    for (size_t i = 0; i < length; ++i) {
        buckets.push_back(current);
        current *= factor;
    }
    return buckets;
}

std::vector<double> metrics::linear_buckets(double start, double width, size_t length) {
    std::vector<double> buckets;
    double current = start;
    for (size_t i = 0; i < length; ++i) {
        buckets.push_back(current);
        current += width;
    }
    return buckets;
}

std::vector<double> metrics::exponential_buckets_range(double min, double max, size_t length) {
    if (length < 1 || min <= 0.0) {
        return {};
    }
    const double factor = std::pow(max / min, 1.0 / (length - 1));
    return exponential_buckets(min, factor, length);
}