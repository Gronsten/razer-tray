#include "tray_app.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace razer {

// Static instance pointer for window procedure
TrayApp* TrayApp::instance = nullptr;

TrayApp::TrayApp(HINSTANCE hInstance)
    : hInstance(hInstance)
    , hwnd(nullptr)
{
    instance = this;
    ZeroMemory(&nid, sizeof(nid));
}

TrayApp::~TrayApp() {
    cleanup();
    instance = nullptr;
}

bool TrayApp::initialize() {
    // Initialize device manager
    if (!device_manager.initialize()) {
        MessageBoxW(nullptr, L"Failed to initialize device manager.", L"Error", MB_ICONERROR | MB_OK);
        return false;
    }

    // Discover devices
    discover_devices();

    if (devices.empty()) {
        MessageBoxW(nullptr,
            L"No Razer devices found.\n\n"
            L"Please ensure your device is connected and powered on.",
            L"No Devices Found",
            MB_ICONWARNING | MB_OK);
        return false;
    }

    // Register window class
    register_window_class();

    // Create hidden window for message processing
    if (!create_window()) {
        return false;
    }

    // Create tray icon
    if (!create_tray_icon()) {
        return false;
    }

    // Start refresh timer
    SetTimer(hwnd, TIMER_REFRESH, REFRESH_INTERVAL, nullptr);

    // Initial battery update
    refresh_battery_status();

    return true;
}

void TrayApp::run() {
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void TrayApp::cleanup() {
    // Kill timer
    if (hwnd) {
        KillTimer(hwnd, TIMER_REFRESH);
    }

    // Remove tray icon
    remove_tray_icon();

    // Close all devices
    devices.clear();

    // Destroy window
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
}

void TrayApp::register_window_class() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;

    RegisterClassW(&wc);
}

bool TrayApp::create_window() {
    hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS,
        L"Razer Tray",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // Message-only window
        nullptr,
        hInstance,
        this
    );

    return hwnd != nullptr;
}

bool TrayApp::create_tray_icon() {
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;

    // Create initial icon (will be updated with battery status)
    nid.hIcon = create_battery_icon(0, false);

    // Set initial tooltip
    std::wstring tooltip = L"Razer Tray - Initializing...";
    wcsncpy_s(nid.szTip, tooltip.c_str(), _TRUNCATE);

    return Shell_NotifyIconW(NIM_ADD, &nid);
}

void TrayApp::update_tray_icon() {
    if (devices.empty() || battery_statuses.empty()) {
        return;
    }

    // Use first device's battery status
    const auto& status = battery_statuses[0];

    // Update icon
    if (nid.hIcon) {
        DestroyIcon(nid.hIcon);
    }
    nid.hIcon = create_battery_icon(status.percentage, status.is_charging);

    // Update tooltip
    std::wstring tooltip = generate_tooltip();
    wcsncpy_s(nid.szTip, tooltip.c_str(), _TRUNCATE);

    // Notify shell
    Shell_NotifyIconW(NIM_MODIFY, &nid);
}

void TrayApp::remove_tray_icon() {
    if (nid.hIcon) {
        DestroyIcon(nid.hIcon);
        nid.hIcon = nullptr;
    }

    Shell_NotifyIconW(NIM_DELETE, &nid);
}

HICON TrayApp::create_battery_icon(int percentage, bool charging) {
    // Create a 16x16 icon with battery visualization
    const int width = 16;
    const int height = 16;

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Create 32-bit RGBA bitmap for proper transparency
    BITMAPV5HEADER bi = {};
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = width;
    bi.bV5Height = -height;  // Top-down DIB
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask   = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask  = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    UINT32* pBits = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcScreen, (BITMAPINFO*)&bi, DIB_RGB_COLORS,
                                    (void**)&pBits, nullptr, 0);

    if (!hbm || !pBits) {
        ReleaseDC(nullptr, hdcScreen);
        DeleteDC(hdcMem);
        return nullptr;
    }

    SelectObject(hdcMem, hbm);

    // Choose color based on battery level (ARGB format)
    UINT32 color;
    if (charging) {
        color = 0xFF00FF00;  // Green when charging
    } else if (percentage >= 75) {
        color = 0xFF00DC00;  // Green
    } else if (percentage >= 50) {
        color = 0xFFFFDC00;  // Yellow
    } else if (percentage >= 25) {
        color = 0xFFFF8C00;  // Orange
    } else {
        color = 0xFFFF0000;  // Red
    }

    // Fill background with transparency
    for (int i = 0; i < width * height; i++) {
        pBits[i] = 0x00000000;  // Fully transparent
    }

    // Draw simple battery icon
    // Battery body (outline)
    for (int y = 3; y < 14; y++) {
        for (int x = 2; x < 14; x++) {
            if (y == 3 || y == 13 || x == 2 || x == 13) {
                pBits[y * width + x] = color;
            }
        }
    }

    // Battery terminal (top)
    for (int y = 1; y < 3; y++) {
        for (int x = 6; x < 10; x++) {
            pBits[y * width + x] = color;
        }
    }

    // Battery fill based on percentage
    int fillHeight = (percentage * 9) / 100;  // 9 pixels max height
    if (fillHeight < 1 && percentage > 0) fillHeight = 1;

    for (int y = 12 - fillHeight; y < 13; y++) {
        for (int x = 3; x < 13; x++) {
            pBits[y * width + x] = color;
        }
    }

    // Create icon from bitmap
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hbm;
    iconInfo.hbmMask = hbm;  // Use same bitmap for mask (alpha channel handles transparency)

    HICON hIcon = CreateIconIndirect(&iconInfo);

    // Cleanup
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    return hIcon;
}

void TrayApp::show_context_menu() {
    HMENU hMenu = CreatePopupMenu();

    // Device info section
    if (!devices.empty() && !battery_statuses.empty()) {
        for (size_t i = 0; i < devices.size(); i++) {
            std::wostringstream oss;

            // Convert product name from string to wstring
            std::string name = devices[i]->product_name();
            std::wstring wname(name.begin(), name.end());

            // If it's a dock, show as "Razer Mouse" since it's reporting the mouse battery
            if (devices[i]->pid() == 0x00A4) {  // Mouse Dock Pro
                wname = L"Razer Mouse";
            }

            oss << wname << L" - "
                << battery_statuses[i].percentage << L"%";

            if (battery_statuses[i].is_charging) {
                oss << L" (Charging)";
            }

            AppendMenuW(hMenu, MF_STRING | MF_DISABLED, ID_MENU_DEVICE_INFO + i, oss.str().c_str());
        }

        AppendMenuW(hMenu, MF_SEPARATOR, ID_MENU_SEPARATOR, nullptr);
    }

    // Actions
    AppendMenuW(hMenu, MF_STRING, ID_MENU_REFRESH, L"Refresh Now");
    AppendMenuW(hMenu, MF_SEPARATOR, ID_MENU_SEPARATOR, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"Exit");

    // Get cursor position
    POINT pt;
    GetCursorPos(&pt);

    // Required for proper menu behavior
    SetForegroundWindow(hwnd);

    // Show menu
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);

    DestroyMenu(hMenu);
}

void TrayApp::handle_menu_command(UINT command_id) {
    switch (command_id) {
        case ID_MENU_REFRESH:
            refresh_battery_status();
            break;

        case ID_MENU_EXIT:
            PostQuitMessage(0);
            break;

        default:
            break;
    }
}

void TrayApp::discover_devices() {
    auto device_infos = device_manager.discover_devices();

    for (const auto& info : device_infos) {
        auto device = device_manager.create_device(info);
        if (device) {
            devices.push_back(std::move(device));
            battery_statuses.push_back(BatteryStatus());
        }
    }
}

void TrayApp::refresh_battery_status() {
    for (size_t i = 0; i < devices.size(); i++) {
        auto& device = devices[i];
        auto status = device->get_battery_status();

        if (status) {
            battery_statuses[i] = *status;
        }
    }

    // Update tray icon
    update_tray_icon();
}

std::wstring TrayApp::generate_tooltip() {
    std::wostringstream oss;

    if (devices.empty() || battery_statuses.empty()) {
        oss << L"Razer Tray - No devices";
        return oss.str();
    }

    // First device
    std::string name = devices[0]->product_name();
    std::wstring wname(name.begin(), name.end());

    // If it's a dock, show as "Razer Mouse" since it's reporting the mouse battery
    if (devices[0]->pid() == 0x00A4) {  // Mouse Dock Pro
        wname = L"Razer Mouse";
    }

    oss << wname << L": "
        << battery_statuses[0].percentage << L"%";

    if (battery_statuses[0].is_charging) {
        oss << L" (Charging)";
    }

    // Additional devices (if any)
    if (devices.size() > 1) {
        oss << L"\n+" << (devices.size() - 1) << L" more device(s)";
    }

    return oss.str();
}

LRESULT CALLBACK TrayApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    TrayApp* app = instance;

    switch (uMsg) {
        case WM_CREATE:
            return 0;

        case WM_TRAYICON:
            switch (lParam) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                    if (app) {
                        app->show_context_menu();
                    }
                    return 0;

                case WM_LBUTTONDBLCLK:
                    if (app) {
                        app->refresh_battery_status();
                    }
                    return 0;
            }
            break;

        case WM_COMMAND:
            if (app) {
                app->handle_menu_command(LOWORD(wParam));
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_REFRESH && app) {
                app->refresh_battery_status();
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

} // namespace razer
