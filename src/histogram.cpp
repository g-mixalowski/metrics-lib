#include "histogram.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void metrics::Histogram::observe(double value) noexcept {
    std::unique_lock lock(mutex_);
    is_cached = false;
    auto &data = *inner_;
    data.sum += value;
    data.count++;
    auto it = std::lower_bound(data.buckets.begin(), data.buckets.end(), value);
    if (it != data.buckets.end()) {
        size_t index = std::distance(data.buckets.begin(), it);
        data.counters[index]++;
    }
}

metrics::Histogram::Snapshot metrics::Histogram::get() const noexcept {
    std::shared_lock lock(mutex_);
    return {inner_->sum, inner_->count, inner_->buckets, inner_->counters};
}

std::string_view metrics::Histogram::name() const noexcept {
    return name_;
}

std::string metrics::Histogram::value_as_str() const {
    if (is_cached) {
        return cached_value;
    }
    Snapshot snapshot = get();

    std::size_t approx_length = 256 + snapshot.buckets.size() * 64;
    std::string result;
    result.reserve(approx_length);
    uint64_t sum = 0;

    result += '{';
    for (std::size_t i = 0; i < snapshot.buckets.size(); ++i) {
        sum += snapshot.counters[i];
        result += "\"";
        result += name_;
        result += "_bucket{le=";
        if (std::isinf(snapshot.buckets[i])) {
            result += "+Inf";
        } else {
            result += std::to_string(snapshot.buckets[i]);
        }
        result += "}\" ";
        result += std::to_string(sum);
        result += " ";
    }

    result += " \"";
    result += name_;
    result += "_sum\" ";
    result += std::to_string(snapshot.sum);
    result += " ";

    result += " \"";
    result += name_;
    result += "_count\" ";
    result += std::to_string(snapshot.count);
    result += "}";

    is_cached = true;
    cached_value = result;
    return result;
}

void metrics::Histogram::reset() noexcept {
    for (auto &counter : inner_->counters) {
        counter = 0;
    }
    inner_->sum = 0;
    inner_->count = 0;
}

std::vector<double>
metrics::exponential_buckets(double start, double factor, size_t length) {
    std::vector<double> buckets;
    buckets.reserve(length);
    double current = start;
    for (size_t i = 0; i < length; ++i) {
        buckets.push_back(current);
        current *= factor;
    }
    return buckets;
}

std::vector<double>
metrics::linear_buckets(double start, double width, size_t length) {
    std::vector<double> buckets;
    buckets.reserve(length);
    double current = start;
    for (size_t i = 0; i < length; ++i) {
        buckets.push_back(current);
        current += width;
    }
    return buckets;
}

std::vector<double>
metrics::exponential_buckets_range(double min, double max, size_t length) {
    if (length < 1 || min <= 0.0) {
        return {};
    }
    const double factor = std::pow(max / min, 1.0 / (length - 1));
    return exponential_buckets(min, factor, length);
}