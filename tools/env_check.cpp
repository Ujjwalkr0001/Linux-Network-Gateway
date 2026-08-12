#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

void check_proc_limits() {
    std::cout << "[+] Inspecting Linux Kernel Environment...\n";
    
    // 1. Check Hardware Concurrency (CPU Cores available to process)
    unsigned int cores = std::thread::hardware_concurrency();
    std::cout << "  - Available CPU Cores: " << cores << "\n";

    // 2. Check Open File Descriptor Limit (ulimit -n) via getrlimit system call
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        std::cout << "  - Max Open File Descriptors (soft limit): " << rl.rlim_cur << "\n";
        std::cout << "  - Max Open File Descriptors (hard limit): " << rl.rlim_max << "\n";
    } else {
        perror("getrlimit failed");
    }

    // 3. Read Local Ephemeral Port Range from /proc virtual filesystem
    std::ifstream port_range_file("/proc/sys/net/ipv4/ip_local_port_range");
    if (port_range_file.is_open()) {
        std::string range;
        std::getline(port_range_file, range);
        std::cout << "  - Ephemeral Port Range (/proc): " << range << "\n";
    } else {
        std::cout << "  - Note: /proc filesystem not accessible (Make sure you are running in Linux / WSL2)\n";
    }
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " PHASE 0: LINUX SYSTEM & TOOLCHAIN CHECK\n";
    std::cout << "=================================================\n";
    
    check_proc_limits();

    // Verify POSIX Threading
    std::cout << "\n[+] Testing POSIX Multithreading (std::thread)...\n";
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([i]() {
            long tid = syscall(SYS_gettid);
            std::cout << "  [Worker " << i << "] Running on Linux Kernel Thread ID (TID): " << tid << "\n";
        });
    }
    for (auto& t : workers) {
        t.join();
    }

    std::cout << "\n[✓] Environment Verification Complete!\n";
    std::cout << "=================================================\n";
    return 0;
}
