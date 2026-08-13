#ifndef BENCHMARKING_SUITE_HPP
#define BENCHMARKING_SUITE_HPP

// ─────────────────────────────────────────────────────────────────────────
// EMPIRICAL PERFORMANCE BENCHMARKING SUITE
//
// Measures throughput (ops/sec) and microsecond latency percentiles:
//   - P50 : Median latency (50th percentile)
//   - P90 : 90th percentile latency
//   - P99 : Tail latency (99th percentile)
//   - Min, Max, Avg latency
// ─────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <cmath>

struct BenchmarkResult {
    std::string name;
    uint64_t    total_ops;
    double      duration_sec;
    double      throughput_ops_sec;
    double      min_us;
    double      p50_us;
    double      p90_us;
    double      p99_us;
    double      max_us;
    double      avg_us;
};

class LatencyTracker {
public:
    LatencyTracker() = default;

    void reserve(size_t n) {
        samples_.reserve(n);
    }

    void record_us(double microseconds) {
        samples_.push_back(microseconds);
    }

    BenchmarkResult compute_results(const std::string& name, double elapsed_sec, uint64_t total_ops) const {
        if (samples_.empty()) {
            return BenchmarkResult{ name, total_ops, elapsed_sec, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        }

        std::vector<double> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());

        size_t n = sorted.size();
        double sum = std::accumulate(sorted.begin(), sorted.end(), 0.0);
        double avg = sum / n;
        double min_val = sorted.front();
        double max_val = sorted.back();

        double p50 = sorted[static_cast<size_t>(n * 0.50)];
        double p90 = sorted[static_cast<size_t>(n * 0.90)];
        double p99 = sorted[std::min(static_cast<size_t>(n * 0.99), n - 1)];

        double throughput = elapsed_sec > 0.0 ? (static_cast<double>(total_ops) / elapsed_sec) : 0.0;

        return BenchmarkResult{
            name, total_ops, elapsed_sec, throughput,
            min_val, p50, p90, p99, max_val, avg
        };
    }

private:
    std::vector<double> samples_;
};

class BenchmarkTimer {
public:
    BenchmarkTimer() : start_(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    double elapsed_sec() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_).count();
    }

    double elapsed_us() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(now - start_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ── Print comparative summary table formatted for Markdown / Terminal ─────
inline void print_benchmark_comparison(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n====================================================================================================\n";
    std::cout << " EMPIRICAL PERFORMANCE BENCHMARK COMPARISON REPORT\n";
    std::cout << "====================================================================================================\n";
    std::cout << std::left 
              << std::setw(28) << "Architecture Paradigm"
              << std::setw(12) << "Total Ops"
              << std::setw(16) << "Throughput (ops/s)"
              << std::setw(10) << "P50 (us)"
              << std::setw(10) << "P90 (us)"
              << std::setw(10) << "P99 (us)"
              << std::setw(10) << "Max (us)"
              << "\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    for (const auto& r : results) {
        std::cout << std::left
                  << std::setw(28) << r.name
                  << std::setw(12) << r.total_ops
                  << std::setw(16) << std::fixed << std::setprecision(1) << r.throughput_ops_sec
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.p50_us
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.p90_us
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.p99_us
                  << std::setw(10) << std::fixed << std::setprecision(2) << r.max_us
                  << "\n";
    }
    std::cout << "====================================================================================================\n\n";
}

#endif // BENCHMARKING_SUITE_HPP
