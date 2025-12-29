#pragma once

#include <windows.h>
#include <shellapi.h>
#include <memory>
#include <vector>
#include <optional>
#include "DeviceMonitor.h"
#include "BatteryIcon.h"
#include "ConfigManager.h"

class TrayApp {
public:
    TrayApp(HINSTANCE hInstance);
    ~TrayApp();

    // Initialize and run the application
    bool initialize();
    void run();
    void cleanup();

    // Window procedure
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // Timer IDs
    static constexpr UINT TIMER_REFRESH = 1;
    static constexpr UINT TIMER_REFRESH_ANIMATION = 2;
    static constexpr UINT TIMER_ANIMATION_STOP = 3;

    // Custom window messages
    static constexpr UINT WM_TRAYICON = WM_USER + 1;

    // Menu IDs
    static constexpr UINT ID_MENU_REFRESH = 1001;
    static constexpr UINT ID_MENU_EXIT = 1002;

    // Refresh interval (5 minutes in milliseconds)
    static constexpr UINT REFRESH_INTERVAL = 5 * 60 * 1000;

    // Animation settings
    static constexpr UINT ANIMATION_INTERVAL = 100;  // 100ms per frame
    static constexpr int ANIMATION_FRAMES = 8;  // Number of rotation steps
    static constexpr UINT ANIMATION_DURATION = 3000;  // Show animation for 3 seconds

    HINSTANCE hInstance;
    HWND hwnd;
    NOTIFYICONDATAW notifyIconData;

    std::unique_ptr<DeviceMonitor> deviceMonitor;
    std::unique_ptr<BatteryIcon> batteryIcon;
    std::vector<std::unique_ptr<RazerDevice>> devices;
    std::optional<Config> config;
    UINT refreshInterval;

    // Animation state
    bool isRefreshing;
    int animationFrame;
    SYSTEMTIME lastRefreshTime;

    // Create hidden window for message processing
    bool createWindow();

    // System tray functions
    bool addTrayIcon();
    void updateTrayIcon();
    void removeTrayIcon();
    void showContextMenu();

    // Device management
    void discoverDevices();
    void refreshDevices();
    void startRefreshAnimation();
    void stopRefreshAnimation();
    void updateRefreshAnimation();

    // Generate tooltip text
    std::wstring getTooltipText();
    std::wstring formatTimestamp(const SYSTEMTIME& time);
};
