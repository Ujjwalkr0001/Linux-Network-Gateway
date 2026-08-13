#include <iostream>
#include <fstream>
#include <cassert>
#include "config/ConfigParser.hpp"

void test_default_configuration() {
    std::cout << "--- Test 1: Default Fallback Configuration ---\n";

    ConfigParser parser;
    // Attempt loading non-existent config file -> should fallback gracefully
    parser.load_from_file("non_existent_file.conf");

    const auto& cfg = parser.config();
    std::cout << "  Loaded Default Parameters:\n";
    cfg.print();

    assert(cfg.port == 8080);
    assert(cfg.thread_pool_size == 4);
    assert(parser.validate() == true);
    std::cout << "  [PASS] Default fallback configuration verified.\n\n";
}

void test_custom_file_configuration() {
    std::cout << "--- Test 2: File-Based Configuration Loading ---\n";

    ConfigParser parser;
    bool loaded = parser.load_from_file("config/gateway.conf");
    assert(loaded == true);

    const auto& cfg = parser.config();
    std::cout << "  Loaded Custom File Parameters:\n";
    cfg.print();

    assert(cfg.port == 8080);
    assert(cfg.thread_pool_size == 8);
    assert(cfg.rate_limit_capacity == 200);
    assert(cfg.log_level == "DEBUG");
    assert(parser.validate() == true);
    std::cout << "  [PASS] Custom file parsing verified successfully.\n\n";
}

void test_config_validation() {
    std::cout << "--- Test 3: Parameter Range & Type Validation ---\n";

    // Create temporary invalid config file
    {
        std::ofstream invalid_file("tmp_invalid.conf");
        invalid_file << "port = 70000\n"; // Invalid port > 65535
        invalid_file << "thread_pool_size = 0\n"; // Invalid 0 workers
    }

    ConfigParser parser;
    parser.load_from_file("tmp_invalid.conf");

    bool is_valid = parser.validate();
    std::cout << "  Validation Result for Invalid Config: " << (is_valid ? "VALID" : "INVALID (Rejected)") << "\n";
    assert(is_valid == false);

    // Cleanup temp file
    std::remove("tmp_invalid.conf");
    std::cout << "  [PASS] Out-of-range validation successfully caught bad settings.\n\n";
}

void test_hot_reloading() {
    std::cout << "--- Test 4: Dynamic Hot-Reloading Simulation ---\n";

    std::string test_conf = "tmp_reload.conf";
    {
        std::ofstream file(test_conf);
        file << "port = 9000\n";
        file << "thread_pool_size = 2\n";
    }

    ConfigParser parser;
    parser.load_from_file(test_conf);
    assert(parser.config().port == 9000);
    assert(parser.config().thread_pool_size == 2);

    // Simulate updating configuration on disk at runtime
    {
        std::ofstream file(test_conf);
        file << "port = 9500\n";
        file << "thread_pool_size = 16\n";
    }

    // Trigger dynamic hot reload
    parser.reload();
    assert(parser.config().port == 9500);
    assert(parser.config().thread_pool_size == 16);

    std::remove(test_conf.c_str());
    std::cout << "  Updated Parameters after Hot-Reload:\n";
    parser.config().print();
    std::cout << "  [PASS] Dynamic runtime hot-reloading verified successfully.\n\n";
}

int main() {
    std::cout << "====================================================================================================\n";
    std::cout << " PHASE 21: RUNTIME CONFIGURATION SUBSYSTEM\n";
    std::cout << "====================================================================================================\n\n";

    test_default_configuration();
    test_custom_file_configuration();
    test_config_validation();
    test_hot_reloading();

    std::cout << "====================================================================================================\n";
    std::cout << " [✓] ALL CONFIGURATION SUBSYSTEM TESTS PASSED!\n";
    std::cout << "====================================================================================================\n";
    return 0;
}
