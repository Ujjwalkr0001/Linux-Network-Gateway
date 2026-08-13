#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

void display_kernel_vs_userspace_architecture() {
    std::cout << "====================================================================================================\n";
    std::cout << " PHASE 23: EDUCATIONAL LINUX KERNEL MODULE DEMO\n";
    std::cout << " (Ring 0 Kernel Space Driver vs Ring 3 User Space Gateway)\n";
    std::cout << "====================================================================================================\n\n";

    std::cout << " Architectural Overview:\n";
    std::cout << "   - Ring 0 (Kernel Space): Loadable Kernel Module ('gateway_stats_mod.ko') running with full HW access.\n";
    std::cout << "   - Ring 3 (User Space)  : Network Gateway Process accessing kernel telemetry via procfs ('/proc/gateway_stats').\n";
    std::cout << "   - Inter-Ring Bridge    : Linux procfs virtual filesystem + System Call Interface (sys_read).\n\n";
}

bool read_proc_gateway_stats() {
    std::string proc_path = "/proc/gateway_stats";
    std::ifstream proc_file(proc_path);

    if (!proc_file.is_open()) {
        std::cout << "[!] Virtual file '" << proc_path << "' not accessible.\n";
        std::cout << "    (Make sure you compile and insert kernel module: cd kernel && make && sudo insmod gateway_stats_mod.ko)\n\n";
        return false;
    }

    std::cout << "[+] Reading live telemetry from Kernel Space (/proc/gateway_stats):\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";
    std::string line;
    while (std::getline(proc_file, line)) {
        std::cout << line << "\n";
    }
    std::cout << "----------------------------------------------------------------------------------------------------\n";
    return true;
}

void fallback_kernel_telemetry_simulation() {
    std::cout << "[+] Simulated Kernel Space Procfs Interface Output (/proc/gateway_stats):\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";
    std::cout << "========================================\n";
    std::cout << " LINUX KERNEL GATEWAY TELEMETRY (/proc/gateway_stats)\n";
    std::cout << "========================================\n";
    std::cout << "  Kernel Ring Privilege : Ring 0 (LKM Driver)\n";
    std::cout << "  Kernel RX Packets     : 1048576\n";
    std::cout << "  Kernel TX Packets     : 1048000\n";
    std::cout << "  Kernel Dropped Packets: 576\n";
    std::cout << "========================================\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n\n";
}

int main() {
    display_kernel_vs_userspace_architecture();

    if (!read_proc_gateway_stats()) {
        fallback_kernel_telemetry_simulation();
    }

    std::cout << "[+] Kernel Module Source Code: kernel/gateway_stats_mod.c\n";
    std::cout << "[+] Kbuild Compilation File   : kernel/Makefile\n\n";
    std::cout << "[✓] PHASE 23 COMPLETE. KERNEL VS USER SPACE ARCHITECTURE DEMONSTRATED.\n";
    return 0;
}
