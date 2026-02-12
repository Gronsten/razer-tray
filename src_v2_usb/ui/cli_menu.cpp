#include "cli_menu.h"
#include <iostream>
#include <iomanip>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <cstdlib>
#endif

namespace razer {

CLIMenu::CLIMenu(DeviceManager& manager)
    : device_manager(manager)
{
}

void CLIMenu::run() {
    clear_screen();
    show_main_menu();
}

void CLIMenu::show_main_menu() {
    while (true) {
        clear_screen();
        print_header("Razer Tray - Main Menu");

        std::cout << "\n";
        std::cout << "  [1] Discover Devices\n";
        std::cout << "  [2] List Configured Devices\n";
        std::cout << "  [3] Test Battery Reading\n";
        std::cout << "  [4] Start System Tray Monitor\n";
        std::cout << "  [5] About\n";
        std::cout << "  [0] Exit\n";
        std::cout << "  [0] Exit\n";
        std::cout << "\n";

        int choice = get_user_choice(0, 5);

        switch (choice) {
            case 1:
                show_discovery_menu();
                break;
            case 2:
                show_device_list();
                pause();
                break;
            case 3:
                show_test_battery();
                pause();
                break;
            case 4:
                std::cout << "\nSystem tray monitor not yet implemented.\n";
                pause();
                break;
            case 5:
                show_about();
                pause();
                break;
            case 0:
                return;
        }
    }
}

void CLIMenu::show_discovery_menu() {
    clear_screen();
    print_header("Device Discovery");

    std::cout << "\nScanning for Razer devices...\n";
    print_separator();

    auto devices = device_manager.discover_devices();

    if (devices.empty()) {
        std::cout << "\nNo Razer devices found.\n";
        std::cout << "\nPlease ensure:\n";
        std::cout << "  - Your Razer device is connected\n";
        std::cout << "  - USB cable is properly seated\n";
        std::cout << "  - Device is powered on (for wireless devices)\n";
        std::cout << "  - WinUSB driver is installed (may require Zadig)\n";
        return;
    }

    std::cout << "\nFound " << devices.size() << " device(s):\n\n";

    for (size_t i = 0; i < devices.size(); i++) {
        display_device_info(devices[i], i + 1);
    }

    print_separator();
    std::cout << "\nDiscovery complete.\n";
}

void CLIMenu::show_device_list() {
    clear_screen();
    print_header("Configured Devices");

    std::cout << "\nConfiguration system not yet implemented.\n";
    std::cout << "Use 'Discover Devices' to see connected devices.\n";
}

void CLIMenu::show_test_battery() {
    clear_screen();
    print_header("Battery Test");

    std::cout << "\nScanning for devices...\n";
    auto devices = device_manager.discover_devices();

    if (devices.empty()) {
        std::cout << "\nNo devices found.\n";
        return;
    }

    std::cout << "\nTesting battery on " << devices.size() << " device(s):\n";
    print_separator();

    for (const auto& info : devices) {
        std::cout << "\nDevice: " << info.product_name << "\n";
        std::cout << "Serial: " << (info.serial.empty() ? "N/A" : info.serial) << "\n";

        auto device = device_manager.create_device(info);
        if (!device) {
            std::cout << "Status: Failed to create device instance\n";
            continue;
        }

        if (!device->open()) {
            std::cout << "Status: Failed to open device\n";
            continue;
        }

        auto battery = device->get_battery_status();
        if (battery) {
            display_battery_status(info, *battery);
        } else {
            std::cout << "Status: Failed to read battery (device may not support battery query)\n";
        }

        device->close();
    }

    print_separator();
}

void CLIMenu::show_about() {
    clear_screen();
    print_header("About Razer Tray");

    std::cout << "\n";
    std::cout << "Razer Tray v2.0.0\n";
    std::cout << "USB-based Razer device monitor for Windows\n";
    std::cout << "\n";
    std::cout << "Features:\n";
    std::cout << "  - Battery monitoring for wireless devices\n";
    std::cout << "  - System tray integration\n";
    std::cout << "  - Device auto-discovery\n";
    std::cout << "  - Multiple device support\n";
    std::cout << "\n";
    std::cout << "Technology:\n";
    std::cout << "  - libusb 1.0 for USB communication\n";
    std::cout << "  - Razer USB protocol (based on openrazer)\n";
    std::cout << "  - Native Windows system tray\n";
    std::cout << "\n";
}

void CLIMenu::print_header(const std::string& title) {
    print_separator();
    std::cout << "  " << title << "\n";
    print_separator();
}

void CLIMenu::print_separator() {
    std::cout << std::string(60, '=') << "\n";
}

int CLIMenu::get_user_choice(int min, int max) {
    int choice;
    while (true) {
        std::cout << "Enter choice [" << min << "-" << max << "]: ";
        std::cin >> choice;

        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice < min || choice > max) {
            std::cout << "Choice out of range. Please try again.\n";
            continue;
        }

        return choice;
    }
}

bool CLIMenu::get_yes_no(const std::string& prompt) {
    char response;
    while (true) {
        std::cout << prompt << " [y/n]: ";
        std::cin >> response;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        response = tolower(response);
        if (response == 'y') return true;
        if (response == 'n') return false;

        std::cout << "Please enter 'y' or 'n'.\n";
    }
}

void CLIMenu::pause() {
    std::cout << "\nPress any key to continue...";
    #ifdef _WIN32
    _getch();
    #else
    std::cin.get();
    #endif
}

void CLIMenu::clear_screen() {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

void CLIMenu::display_device_info(const DeviceInfo& info, int index) {
    std::cout << "[" << index << "] " << info.product_name << "\n";
    std::cout << "    VID:PID  : " << std::hex << std::setw(4) << std::setfill('0')
              << info.vid << ":" << std::setw(4) << info.pid << std::dec << "\n";
    std::cout << "    Serial   : " << (info.serial.empty() ? "N/A" : info.serial) << "\n";
    std::cout << "    Type     : " << DeviceManager::get_device_type(info.pid) << "\n";
    std::cout << "\n";
}

void CLIMenu::display_battery_status(const DeviceInfo& info, const BatteryStatus& status) {
    std::cout << "Battery  : " << status.percentage << "%";

    if (status.is_charging) {
        std::cout << " (Charging)";
    }

    std::cout << "\n";

    // Visual battery bar
    std::cout << "Status   : [";
    int bars = status.percentage / 5;  // 20 bars for 100%
    for (int i = 0; i < 20; i++) {
        if (i < bars) {
            std::cout << "=";
        } else {
            std::cout << " ";
        }
    }
    std::cout << "] ";

    // Color-coded status text (using console colors on Windows)
    if (status.percentage >= 75) {
        std::cout << "Excellent";
    } else if (status.percentage >= 50) {
        std::cout << "Good";
    } else if (status.percentage >= 25) {
        std::cout << "Low";
    } else {
        std::cout << "Critical";
    }

    std::cout << "\n";
}


} // namespace razer
