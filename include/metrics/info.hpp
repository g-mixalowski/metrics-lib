#ifndef INFO_HPP_
#define INFO_HPP_

#include "metric.hpp"
#include <string>

namespace metrics {

template <typename S = std::string>
class Info : public Metric {
public:
    explicit Info(std::string name, std::vector<std::pair<S, S>> labels): name_(name), labels_(std::move(labels)) {}

    const std::vector<std::pair<S, S>>& labels() const {
        return labels_;
    }

    std::string name() const override {
        return name_;
    }

    std::string value_as_str() const override {
        std::ostringstream oss;
        oss << "{";
        for (int i = 0; i < labels_.size(); ++i) {
            auto &[label, value] = labels_[i];
            if (i > 0) oss << ",";
            oss << label << "=\"" << value << "\"";
        }
        oss << "}";
        return oss.str();
    }

    void reset() override {
        return;
    }

private:
    const std::vector<std::pair<S, S>> labels_;
    std::string name_;
};

} // namespace metrics

#endif