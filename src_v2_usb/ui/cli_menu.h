#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../devices/device_manager.h"

namespace razer {

// CLI Menu system for interactive device selection and configuration
class CLIMenu {
public:
    CLIMenu(DeviceManager& manager);

    // Main menu entry point
    void run();

    // Individual menu screens
    void show_main_menu();
    void show_discovery_menu();
    void show_device_list();
    void show_test_battery();
    void show_about();

private:
    DeviceManager& device_manager;

    // Menu helpers
    void print_header(const std::string& title);
    void print_separator();
    int get_user_choice(int min, int max);
    bool get_yes_no(const std::string& prompt);
    void pause();
    void clear_screen();

    // Device display helpers
    void display_device_info(const DeviceInfo& info, int index);
    void display_battery_status(const DeviceInfo& info, const BatteryStatus& status);
};

} // namespace razer
