#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

#include "devices/device_manager.h"
#include "ui/cli_menu.h"
#include "ui/tray_app.h"

// Version information
constexpr const char* VERSION = "2.0.0";
constexpr const char* BUILD_DATE = __DATE__;

void print_version() {
    std::cout << "Razer Tray v" << VERSION << "\n";
    std::cout << "Built: " << BUILD_DATE << "\n";
}

void print_help() {
    std::cout << "Usage: razer-tray [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --discover, -d        Run device discovery and display results\n";
    std::cout << "  --list, -l            List currently configured devices\n";
    std::cout << "  --test-battery        Test battery query on all devices\n";
    std::cout << "  --menu, -m            Show interactive CLI menu\n";
    std::cout << "  --tray                Start system tray monitor (default)\n";
    std::cout << "  --version, -v         Show version information\n";
    std::cout << "  --help, -h            Show this help message\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  razer-tray --discover      # Scan for devices\n";
    std::cout << "  razer-tray --test-battery  # Test battery reading\n";
    std::cout << "  razer-tray --menu          # Interactive menu\n";
    std::cout << "  razer-tray                 # Start tray monitor\n";
    std::cout << "\n";
}

void run_discovery(razer::DeviceManager& manager) {
    std::cout << "Scanning for Razer devices...\n";
    auto devices = manager.discover_devices();

    if (devices.empty()) {
        std::cout << "\nNo Razer devices found.\n";
        return;
    }

    std::cout << "\nFound " << devices.size() << " device(s):\n\n";

    for (size_t i = 0; i < devices.size(); i++) {
        const auto& info = devices[i];
        std::cout << "[" << (i + 1) << "] " << info.product_name << "\n";
        std::cout << "    VID:PID : " << std::hex << info.vid << ":" << info.pid << std::dec << "\n";
        std::cout << "    Serial  : " << (info.serial.empty() ? "N/A" : info.serial) << "\n";
        std::cout << "    Type    : " << razer::DeviceManager::get_device_type(info.pid) << "\n";
        std::cout << "\n";
    }
}

void run_test_battery(razer::DeviceManager& manager) {
    std::cout << "Scanning for devices...\n";
    auto devices = manager.discover_devices();

    if (devices.empty()) {
        std::cout << "No devices found.\n";
        return;
    }

    std::cout << "\nTesting battery on " << devices.size() << " device(s):\n\n";

    for (const auto& info : devices) {
        std::cout << "Device: " << info.product_name << "\n";
        std::cout << "Serial: " << (info.serial.empty() ? "N/A" : info.serial) << "\n";

        auto device = manager.create_device(info);
        if (!device) {
            std::cout << "Status: Failed to create device\n\n";
            continue;
        }

        if (!device->open()) {
            std::cout << "Status: Failed to open device\n\n";
            continue;
        }

        auto battery = device->get_battery_status();
        if (battery) {
            std::cout << "Battery: " << battery->percentage << "%";
            if (battery->is_charging) {
                std::cout << " (Charging)";
            }
            std::cout << "\n";

            // Visual bar
            std::cout << "Status : [";
            int bars = battery->percentage / 5;
            for (int i = 0; i < 20; i++) {
                std::cout << (i < bars ? "=" : " ");
            }
            std::cout << "]\n";
        } else {
            std::cout << "Status: Failed to read battery\n";
        }

        std::cout << "\n";
        device->close();
    }
}

void run_list(razer::DeviceManager& manager) {
    std::cout << "Configuration system not yet implemented.\n";
    std::cout << "Use --discover to see connected devices.\n";
}

void run_tray(HINSTANCE hInstance) {
    #ifdef _WIN32
    razer::TrayApp app(hInstance);

    if (!app.initialize()) {
        std::cerr << "Failed to initialize tray application.\n";
        return;
    }

    // Run message loop (blocks until exit)
    app.run();
    #else
    std::cout << "System tray is only supported on Windows.\n";
    #endif
}

int main(int argc, char* argv[]) {
    #ifdef _WIN32
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    #endif

    // Parse command line arguments
    if (argc == 1) {
        // No arguments - default to tray mode
        #ifdef _WIN32
        run_tray(hInstance);
        #else
        std::cout << "Use --help for usage information.\n";
        #endif
        return 0;
    }

    std::string arg = argv[1];

    if (arg == "--tray") {
        #ifdef _WIN32
        run_tray(hInstance);
        #else
        std::cout << "System tray is only supported on Windows.\n";
        #endif
    }
    else if (arg == "--version" || arg == "-v") {
        print_version();
    }
    else if (arg == "--help" || arg == "-h") {
        print_help();
    }
    else {
        // Initialize device manager for CLI commands
        razer::DeviceManager manager;
        if (!manager.initialize()) {
            std::cerr << "Failed to initialize device manager.\n";
            return 1;
        }

        if (arg == "--discover" || arg == "-d") {
            run_discovery(manager);
        }
        else if (arg == "--list" || arg == "-l") {
            run_list(manager);
        }
        else if (arg == "--test-battery") {
            run_test_battery(manager);
        }
        else if (arg == "--menu" || arg == "-m") {
            razer::CLIMenu menu(manager);
            menu.run();
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return 1;
        }
    }

    return 0;
}
