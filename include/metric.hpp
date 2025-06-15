#ifndef METRIC_HPP_
#define METRIC_HPP_

#include <string>
#include <string_view>

namespace metrics {

class Metric {
public:
    virtual ~Metric() = default;
    virtual std::string_view name() const = 0;
    virtual std::string value_as_str() const = 0;
    virtual void reset() = 0;
};

}  // namespace metrics

#endif