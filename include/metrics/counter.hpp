#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <memory>
#include <string_view>
#include <type_traits>
#include "metric.hpp"

namespace metrics {

template <typename N = uint64_t, typename A = std::atomic<N>>
class Counter : public Metric {
public:
    Counter(const char *name) noexcept
        : name_(name), inner_(std::make_shared<A>()) {
    }

    template <
        typename S,
        typename = std::enable_if_t<std::is_convertible_v<S, std::string>>>
    Counter(S &&name)
        : name_(std::forward<S>(name)), inner_(std::make_shared<A>()) {
    }

    Counter(const Counter &) = default;
    Counter(Counter &&) = default;
    Counter &operator=(const Counter &) = default;
    Counter &operator=(Counter &&) = default;

    N inc() noexcept {
        return inc_by(N{1});
    }

    N inc_by(N v) {
        return inner_->fetch_add(v, std::memory_order_relaxed);
    }

    N get() const noexcept {
        return inner_->load(std::memory_order_relaxed);
    }

    std::shared_ptr<A> inner() const noexcept {
        return inner_;
    }

    std::string_view name() const noexcept override {
        return name_;
    }

    std::string value_as_str() const override {
        if constexpr (std::is_arithmetic_v<N>) {
            return std::to_string(get());
        } else {
            std::ostringstream oss;
            oss << get();
            return std::move(oss).str();
        }
    }

    void reset() noexcept override {
        inner_->store(N{}, std::memory_order_relaxed);
    }

private:
    const std::string name_;
    std::shared_ptr<A> inner_;
};

template <typename N = uint64_t>
class ConstCounter : public Metric {
public:
    template <
        typename S,
        typename V,
        typename = std::enable_if_t<std::is_convertible_v<S, std::string>>,
        typename = std::enable_if_t<std::is_convertible_v<V, N>>>
    explicit ConstCounter(S &&name, V &&value)
        : name_(std::forward<S>(name)), value_(std::forward<V>(value)) {
    }

    ConstCounter(const ConstCounter &) = default;
    ConstCounter(ConstCounter &&) = default;
    ConstCounter &operator=(const ConstCounter &) = default;
    ConstCounter &operator=(ConstCounter &&) = default;

    N get() const noexcept {
        return value_;
    }

    std::string_view name() const noexcept override {
        return name_;
    }

    std::string value_as_str() const override {
        if constexpr (std::is_arithmetic_v<N>) {
            return std::to_string(get());
        } else {
            std::ostringstream oss;
            oss << get();
            return std::move(oss).str();
        }
    }

    void reset() noexcept override {
        return;
    }

private:
    const std::string name_;
    const N value_;
};

}  // namespace metrics

#endif