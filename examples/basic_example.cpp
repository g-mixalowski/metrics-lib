#include <metrics/collector.hpp>
#include <metrics/counter.hpp>
#include <metrics/gauge.hpp>
#include <metrics/histogram.hpp>
#include <metrics/info.hpp>
#include <thread>
#include <random>
#include <chrono>
#include <iostream>

using namespace metrics;

int main() {
    // 0. Collector
    MetricsCollector collector;

    // 1. Counter
    auto requests_counter = std::make_shared<Counter<>>("http_requests");
    collector.register_metric(requests_counter);
    
    // 2. Gauge
    auto cpu_usage = std::make_shared<Gauge<double>>("cpu_usage");
    collector.register_metric(cpu_usage);
    
    // 3. Histogram
    auto response_time = std::make_shared<Histogram>(
        "response_time",
        exponential_buckets(0.01, 2.0, 10)
    );
    collector.register_metric(response_time);
    
    // 4. Info
    auto app_info = std::make_shared<Info<>>(
        "app_info",
        std::vector<std::pair<std::string, std::string>>{
            {"version", "1.2.3"},
            {"environment", "production"}
        }
    );
    collector.register_metric(app_info);
    
    // 5. ConstCounter / ConstGauge
    auto startup_time = std::make_shared<ConstCounter<>>(
        "startup_time",
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    collector.register_metric(startup_time);

    std::vector<std::thread> workers;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> dist(1.0);
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&] {
            for (int j = 0; j < 500; ++j) {
                requests_counter->inc();
                cpu_usage->set(0.75 + static_cast<double>(j % 10) / 40.0);
                
                double latency = dist(gen);
                response_time->observe(latency);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    std::thread flusher([&] {
        for (int i = 0; i < 5; ++i) {
            collector.flush("metrics.log");
            std::cout << "Metrics flushed at " << i << " seconds\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    
    for (auto& t : workers) t.join();
    flusher.join();
    collector.flush("metrics.log");
    std::cout << "All the metrics written to metrics.log\n";
    return 0;
}