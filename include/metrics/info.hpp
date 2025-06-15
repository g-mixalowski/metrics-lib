#ifndef INFO_HPP_
#define INFO_HPP_

#include <string>
#include <string_view>
#include <vector>
#include "metric.hpp"

namespace metrics {

class Info : public Metric {
public:
    template <
        typename S = std::string,
        typename L,
        typename = std::enable_if<std::is_convertible_v<S, std::string>>,
        typename = std::enable_if<std::is_convertible_v<
            L,
            std::vector<std::pair<std::string, std::string>>>>>
    explicit Info(S &&name, L &&labels)
        : name_(std::forward<S>(name)),
          labels_(std::forward<L>(labels)),
          is_cached_{false} {
    }

    Info(const Info &) = default;
    Info(Info &&) = default;
    Info &operator=(const Info &) = default;
    Info &operator=(Info &&) = default;

    const std::vector<std::pair<std::string, std::string>> &labels(
    ) const noexcept {
        return labels_;
    }

    std::string_view name() const noexcept override {
        return name_;
    }

    std::string value_as_str() const override {
        if (is_cached_) {
            return cached_value_;
        }

        std::size_t length = 2;
        for (const auto &[label, value] : labels_) {
            length += label.size() + value.size() + 5;
        }
        length -= 1;

        std::string result;
        result.reserve(length);

        result += '{';
        for (std::size_t i = 0; i < labels_.size(); ++i) {
            const auto &[label, value] = labels_[i];
            if (i > 0) {
                result += ",";
            }
            result += label;
            result += "=\"";
            result += value;
            result += '"';
        }
        result += '}';

        cached_value_ = result;
        is_cached_ = true;

        return result;
    }

    void reset() noexcept override {
        return;
    }

private:
    const std::vector<std::pair<std::string, std::string>> labels_;
    std::string name_;
    mutable bool is_cached_;
    mutable std::string cached_value_;
};

}  // namespace metrics

#endif