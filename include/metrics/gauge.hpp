#ifndef GAUGE_HPP_
#define GAUGE_HPP_

#include "metric.hpp"
#include <memory>

namespace metrics {

template <typename N = uint64_t, typename A = std::atomic<N>>
class Gauge : public Metric {
public:
    Gauge(std::string name) : name_(name), inner_(std::make_shared<A>()) {}

    N inc() {
        return inc_by(N{1});
    }

    N inc_by(N v) {
        return inner_->fetch_add(v, std::memory_order_relaxed);
    }

    N dec() {
        return dec_by(N{1});
    }

    N dec_by(N v) {
        return inner_->fetch_sub(v, std::memory_order_relaxed);
    }

    N set(N v) {
        return *inner_->store(v);
    }

    N get() const {
        return inner_->load(std::memory_order_relaxed);
    }

    std::shared_ptr<A> inner() const {
        return inner_;
    }

    std::string name() const override {
        return name_;
    }

    std::string value_as_str() const override {
        std::ostringstream oss;
        oss << *inner_;
        return oss.str();
    }

    void reset() override {
        set(N{});
    }

private:
    std::string name_;
    std::shared_ptr<A> inner_;
};

template <typename N = uint64_t>
class ConstGauge : public Metric {
public:
    explicit ConstGauge(std::string name, N value) : name_(name), value_(value) {}
    
    N get() const { 
        return value_; 
    }    
    
    std::string name() const {
        return name_;
    }

    std::string value_as_str() const {
        std::ostringstream oss;
        oss << *value_;
        return oss.str();
    }

    void reset() {
        return;
    }

private:
    std::string name_;
    N value_;
};

}

#endif