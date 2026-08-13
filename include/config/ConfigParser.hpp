#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

// ─────────────────────────────────────────────────────────────────────────
// RUNTIME CONFIGURATION SUBSYSTEM
//
// Decouples compile-time settings from operational parameters.
// Parses file-based key-value configuration (`gateway.conf`) with support
// for defaults, validation, and dynamic hot-reloading.
// ─────────────────────────────────────────────────────────────────────────

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <cstdint>

struct GatewayConfig {
    uint16_t    port = 8080;
    size_t      thread_pool_size = 4;
    int64_t     rate_limit_capacity = 100;
    int64_t     rate_limit_refill_rate = 50;
    std::string log_level = "INFO";
    std::string shared_secret = "qualcomm_gateway_s3cr3t";
    std::string log_file = "gateway.log";

    void print() const {
        std::cout << "  - Listening Port       : " << port << "\n";
        std::cout << "  - Thread Pool Workers  : " << thread_pool_size << "\n";
        std::cout << "  - Rate Limit Capacity  : " << rate_limit_capacity << " tokens\n";
        std::cout << "  - Rate Limit Refill    : " << rate_limit_refill_rate << " tokens/sec\n";
        std::cout << "  - Log Level            : " << log_level << "\n";
        std::cout << "  - Shared Auth Secret   : " << shared_secret << "\n";
        std::cout << "  - Log File Output      : " << log_file << "\n";
    }
};

class ConfigParser {
public:
    ConfigParser() = default;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    bool load_from_file(const std::string& filepath) {
        filepath_ = filepath;
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "[!] Config file not found at '" << filepath << "'. Using default settings.\n";
            return false;
        }

        std::string line;
        int line_num = 0;

        while (std::getline(file, line)) {
            line_num++;
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            auto equal_pos = line.find('=');
            if (equal_pos == std::string::npos) {
                std::cerr << "[!] Config warning on line " << line_num << ": Missing '=' delimiter\n";
                continue;
            }

            std::string key = trim(line.substr(0, equal_pos));
            std::string val = trim(line.substr(equal_pos + 1));

            if (!key.empty()) {
                kv_store_[key] = val;
            }
        }

        apply_kv();
        std::cout << "[+] Successfully loaded configuration from '" << filepath << "'\n";
        return true;
    }

    bool reload() {
        if (filepath_.empty()) return false;
        std::cout << "[+] Hot-reloading configuration from '" << filepath_ << "'...\n";
        return load_from_file(filepath_);
    }

    const GatewayConfig& config() const { return config_; }

    bool validate() const {
        if (config_.port == 0 || config_.port > 65535) {
            std::cerr << "[!] Invalid port configuration: " << config_.port << "\n";
            return false;
        }
        if (config_.thread_pool_size == 0 || config_.thread_pool_size > 128) {
            std::cerr << "[!] Invalid thread pool size: " << config_.thread_pool_size << "\n";
            return false;
        }
        if (config_.rate_limit_capacity <= 0 || config_.rate_limit_refill_rate <= 0) {
            std::cerr << "[!] Invalid rate limit parameters\n";
            return false;
        }
        return true;
    }

private:
    void apply_kv() {
        if (kv_store_.count("port")) config_.port = static_cast<uint16_t>(std::stoi(kv_store_["port"]));
        if (kv_store_.count("thread_pool_size")) config_.thread_pool_size = std::stoul(kv_store_["thread_pool_size"]);
        if (kv_store_.count("rate_limit_capacity")) config_.rate_limit_capacity = std::stoll(kv_store_["rate_limit_capacity"]);
        if (kv_store_.count("rate_limit_refill_rate")) config_.rate_limit_refill_rate = std::stoll(kv_store_["rate_limit_refill_rate"]);
        if (kv_store_.count("log_level")) config_.log_level = kv_store_["log_level"];
        if (kv_store_.count("shared_secret")) config_.shared_secret = kv_store_["shared_secret"];
        if (kv_store_.count("log_file")) config_.log_file = kv_store_["log_file"];
    }

    std::string filepath_;
    std::unordered_map<std::string, std::string> kv_store_;
    GatewayConfig config_;
};

#endif // CONFIG_PARSER_HPP
