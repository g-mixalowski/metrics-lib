#include <unistd.h>
#include <fstream>
#include <iostream>
#include <metrics/collector.hpp>
#include <metrics/counter.hpp>
#include <metrics/gauge.hpp>
#include <metrics/histogram.hpp>
#include <metrics/info.hpp>
#include <sstream>
#include <string>
#include <thread>

using namespace metrics;

class SystemMonitor {
public:
    SystemMonitor()
        : cpu_usage(std::make_shared<Gauge<>>("cpu_usage")),
          mem_usage(std::make_shared<Gauge<>>("memory_usage")),
          disk_io(std::make_shared<Counter<>>("disk_io_bytes")),
          net_traffic(std::make_shared<Counter<>>("network_traffic_bytes")),
          sys_info(std::make_shared<Info>(
              "system_info",
              std::vector<std::pair<std::string, std::string>>{
                  {"os", get_os_name()},
                  {"cpu_cores", std::to_string(sysconf(_SC_NPROCESSORS_ONLN))}}
          )),
          collector() {
        collector.register_metric(cpu_usage);
        collector.register_metric(mem_usage);
        collector.register_metric(disk_io);
        collector.register_metric(net_traffic);
        collector.register_metric(sys_info);

        prev_cpu_stats = read_cpu_stats();
        prev_net_stats = read_net_stats();
    }

    void start() {
        running = true;
        monitor_thread = std::thread([this] { monitor(); });
    }

    void stop() {
        running = false;
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        collector.flush("system_metrics.log");
    }

private:
    void monitor() {
        while (running) {
            update_cpu_metrics();
            update_memory_metrics();
            update_network_metrics();
            collector.flush("system_metrics.log");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    void update_cpu_metrics() {
        auto current_stats = read_cpu_stats();
        double total_diff = 0.0;
        for (size_t i = 0; i < current_stats.size(); ++i) {
            total_diff +=
                static_cast<double>(current_stats[i] - prev_cpu_stats[i]);
        }

        if (total_diff > 0) {
            double idle_diff = current_stats[3] - prev_cpu_stats[3];
            double usage = 100.0 * (1.0 - idle_diff / total_diff);
            cpu_usage->set(usage);
        }

        prev_cpu_stats = current_stats;
    }

    void update_memory_metrics() {
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        long total_mem = 0;
        long free_mem = 0;
        long available_mem = 0;

        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                total_mem = parse_mem_line(line);
            } else if (line.find("MemFree:") == 0) {
                free_mem = parse_mem_line(line);
            } else if (line.find("MemAvailable:") == 0) {
                available_mem = parse_mem_line(line);
            }
        }

        if (total_mem > 0) {
            double usage =
                100.0 * (1.0 - static_cast<double>(available_mem) / total_mem);
            mem_usage->set(usage);
        }
    }

    void update_network_metrics() {
        auto current_stats = read_net_stats();
        long rx_diff = current_stats.first - prev_net_stats.first;
        long tx_diff = current_stats.second - prev_net_stats.second;

        net_traffic->inc_by(rx_diff + tx_diff);
        prev_net_stats = current_stats;
    }

    std::vector<long> read_cpu_stats() {
        std::ifstream stat("/proc/stat");
        std::string line;
        std::vector<long> stats;

        if (std::getline(stat, line) && line.substr(0, 3) == "cpu") {
            std::istringstream iss(line.substr(5));
            long value;
            while (iss >> value) {
                stats.push_back(value);
            }
        }
        return stats;
    }

    std::pair<long, long> read_net_stats() {
        std::ifstream net("/proc/net/dev");
        std::string line;
        long rx_total = 0;
        long tx_total = 0;

        std::getline(net, line);
        std::getline(net, line);
        while (std::getline(net, line)) {
            std::istringstream iss(line);
            std::string iface;
            iss >> iface;

            if (iface.find(":") != std::string::npos) {
                iface = iface.substr(0, iface.size() - 1);

                if (iface == "lo" ||
                    iface.find("docker") != std::string::npos) {
                    continue;
                }

                long rx, tx;
                iss >> rx;
                for (int i = 0; i < 7; ++i) {
                    long dummy;
                    iss >> dummy;
                }
                iss >> tx;
                rx_total += rx;
                tx_total += tx;
            }
        }

        return {rx_total, tx_total};
    }

    long parse_mem_line(const std::string &line) {
        auto pos = line.find(':');
        if (pos != std::string::npos) {
            std::string value_str = line.substr(pos + 1);
            size_t endpos;
            long value = std::stol(value_str, &endpos);
            if (endpos < value_str.size() &&
                value_str.find("kB") != std::string::npos) {
                return value * 1024;
            }
            return value;
        }
        return 0;
    }

    static std::string get_os_name() {
        std::ifstream os_release("/etc/os-release");
        std::string line;
        while (std::getline(os_release, line)) {
            if (line.find("PRETTY_NAME") == 0) {
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string name = line.substr(pos + 1);
                    if (name.size() >= 2 && name[0] == '"' &&
                        name[name.size() - 1] == '"') {
                        return name.substr(1, name.size() - 2);
                    }
                    return name;
                }
            }
        }
        return "Linux";
    }

    std::shared_ptr<Gauge<>> cpu_usage;
    std::shared_ptr<Gauge<>> mem_usage;
    std::shared_ptr<Counter<>> disk_io;
    std::shared_ptr<Counter<>> net_traffic;
    std::shared_ptr<Info> sys_info;
    MetricsCollector collector;

    std::vector<long> prev_cpu_stats;
    std::pair<long, long> prev_net_stats;
    std::atomic<bool> running{false};
    std::thread monitor_thread;
};

int main() {
    SystemMonitor monitor;
    monitor.start();
    std::this_thread::sleep_for(std::chrono::minutes(1));
    monitor.stop();
    std::cout << "System monitoring completed. All the metrics saved to "
                 "system_metrics.log\n";
    return 0;
}