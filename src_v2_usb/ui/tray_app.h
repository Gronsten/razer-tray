#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <memory>
#include "../devices/device_manager.h"
#include "../devices/razer_device.h"

namespace razer {

// System tray application
class TrayApp {
public:
    TrayApp(HINSTANCE hInstance);
    ~TrayApp();

    // Initialize and run
    bool initialize();
    void run();
    void cleanup();

    // Window procedure (static)
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    // Window class name
    static constexpr const wchar_t* WINDOW_CLASS = L"RazerTrayWindowClass";

    // Custom messages
    static constexpr UINT WM_TRAYICON = WM_USER + 1;

    // Timer IDs
    static constexpr UINT TIMER_REFRESH = 1;

    // Menu IDs
    static constexpr UINT ID_MENU_DEVICE_INFO = 1001;
    static constexpr UINT ID_MENU_REFRESH = 1002;
    static constexpr UINT ID_MENU_SEPARATOR = 1003;
    static constexpr UINT ID_MENU_EXIT = 1004;

    // Refresh interval (5 minutes)
    static constexpr UINT REFRESH_INTERVAL = 5 * 60 * 1000;

    HINSTANCE hInstance;
    HWND hwnd;
    NOTIFYICONDATAW nid;

    DeviceManager device_manager;
    std::vector<std::unique_ptr<RazerDevice>> devices;
    std::vector<BatteryStatus> battery_statuses;

    // Window management
    bool create_window();
    void register_window_class();

    // Tray icon management
    bool create_tray_icon();
    void update_tray_icon();
    void remove_tray_icon();
    HICON create_battery_icon(int percentage, bool charging);
    HICON load_icon_resource(int icon_id);

    // Menu management
    void show_context_menu();
    void handle_menu_command(UINT command_id);

    // Device management
    void discover_devices();
    void refresh_battery_status();

    // Tooltip generation
    std::wstring generate_tooltip();

    // Instance pointer for window procedure
    static TrayApp* instance;
};

} // namespace razer
