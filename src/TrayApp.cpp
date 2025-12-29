#include "TrayApp.h"
#include <string>
#include <algorithm>
#include <sstream>
#include <cmath>

static const wchar_t* WINDOW_CLASS_NAME = L"RazerBatteryTrayClass";
static const wchar_t* WINDOW_TITLE = L"Razer Battery Tray";

TrayApp::TrayApp(HINSTANCE hInst)
    : hInstance(hInst)
    , hwnd(nullptr)
    , batteryIcon(std::make_unique<BatteryIcon>())
    , config(std::nullopt)
    , refreshInterval(5 * 60 * 1000)  // Default 5 minutes
    , isRefreshing(false)
    , animationFrame(0)
{
    ZeroMemory(&notifyIconData, sizeof(notifyIconData));
    notifyIconData.cbSize = sizeof(NOTIFYICONDATAW);
    ZeroMemory(&lastRefreshTime, sizeof(lastRefreshTime));

    // Try to load config
    ConfigManager configMgr;
    config = configMgr.loadConfig();

    if (config.has_value()) {
        // Config loaded successfully
        refreshInterval = config->refreshInterval * 1000;
        deviceMonitor = std::make_unique<DeviceMonitor>(config.value());
    } else {
        // No config found - use defaults and try to save for next run
        config = configMgr.getDefaultConfig();
        refreshInterval = config->refreshInterval * 1000;

        // Try to save default config (fail silently if permissions issue)
        configMgr.saveConfig(config.value());

        // Create DeviceMonitor with default config
        deviceMonitor = std::make_unique<DeviceMonitor>(config.value());
    }
}

TrayApp::~TrayApp() {
    cleanup();
}

bool TrayApp::initialize() {
    if (!createWindow()) {
        return false;
    }

    if (!addTrayIcon()) {
        return false;
    }

    // Initial device discovery and update
    discoverDevices();
    refreshDevices();
    updateTrayIcon();

    // Set up auto-refresh timer (use configured interval)
    SetTimer(hwnd, TIMER_REFRESH, refreshInterval, nullptr);

    return true;
}

bool TrayApp::createWindow() {
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    // Create hidden window for message processing
    hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        WINDOW_TITLE,
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // Message-only window
        nullptr,
        hInstance,
        this  // Pass 'this' pointer via lpParam
    );

    return hwnd != nullptr;
}

bool TrayApp::addTrayIcon() {
    notifyIconData.hWnd = hwnd;
    notifyIconData.uID = 1;
    notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIconData.uCallbackMessage = WM_TRAYICON;

    // Create initial icon
    HICON icon = batteryIcon->createBatteryIcon(std::nullopt);
    notifyIconData.hIcon = icon;

    wcscpy_s(notifyIconData.szTip, L"Razer Tray - Initializing...");

    bool result = Shell_NotifyIconW(NIM_ADD, &notifyIconData);

    if (icon) {
        DestroyIcon(icon);
    }

    return result;
}

void TrayApp::updateTrayIcon() {
    // Filter for connected devices only
    std::vector<RazerDevice*> connectedDevices;
    for (const auto& device : devices) {
        if (device->isConnected) {
            connectedDevices.push_back(device.get());
        }
    }

    // Update icon based on device state
    HICON newIcon = nullptr;

    if (connectedDevices.empty()) {
        // No devices connected - show gray icon
        newIcon = batteryIcon->createBatteryIcon(std::nullopt);
        wcscpy_s(notifyIconData.szTip, L"Razer Tray - No devices connected");
    } else {
        // Find device with lowest battery
        auto lowestBattery = std::min_element(
            connectedDevices.begin(),
            connectedDevices.end(),
            [](const RazerDevice* a, const RazerDevice* b) {
                if (!a->batteryLevel.has_value()) return false;
                if (!b->batteryLevel.has_value()) return true;
                return a->batteryLevel.value() < b->batteryLevel.value();
            }
        );

        if (lowestBattery != connectedDevices.end() && (*lowestBattery)->batteryLevel.has_value()) {
            newIcon = batteryIcon->createBatteryIcon((*lowestBattery)->batteryLevel);
        } else {
            newIcon = batteryIcon->createBatteryIcon(std::nullopt);
        }

        // Generate tooltip
        std::wstring tooltip = getTooltipText();
        wcscpy_s(notifyIconData.szTip, tooltip.c_str());
    }

    // Update icon
    if (newIcon) {
        HICON oldIcon = notifyIconData.hIcon;
        notifyIconData.hIcon = newIcon;
        Shell_NotifyIconW(NIM_MODIFY, &notifyIconData);

        if (oldIcon) {
            DestroyIcon(oldIcon);
        }
    }
}

void TrayApp::removeTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &notifyIconData);

    if (notifyIconData.hIcon) {
        DestroyIcon(notifyIconData.hIcon);
        notifyIconData.hIcon = nullptr;
    }
}

void TrayApp::showContextMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_MENU_REFRESH, L"Refresh Now");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, ID_MENU_EXIT, L"Exit");

    // Get cursor position
    POINT pt;
    GetCursorPos(&pt);

    // Required for popup menu to work correctly
    SetForegroundWindow(hwnd);

    TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);

    DestroyMenu(menu);
}

void TrayApp::discoverDevices() {
    devices = deviceMonitor->enumerateRazerDevices();
}

void TrayApp::refreshDevices() {
    // Start animation (will run for 3 seconds)
    startRefreshAnimation();

    // Perform refresh (instant, but animation continues)
    deviceMonitor->updateDeviceInfo(devices);

    // Capture timestamp
    GetLocalTime(&lastRefreshTime);

    // Update icon data (but keep showing animation)
    // Note: updateTrayIcon() is NOT called here - animation handles icon updates
    // The final icon update happens when animation stops

    // Schedule animation to stop after 3 seconds
    SetTimer(hwnd, TIMER_ANIMATION_STOP, ANIMATION_DURATION, nullptr);
}

std::wstring TrayApp::getTooltipText() {
    std::wstringstream ss;
    ss << L"Razer Tray";

    // Add timestamp right under title if we've refreshed at least once
    if (lastRefreshTime.wYear != 0) {
        ss << L"\n" << formatTimestamp(lastRefreshTime);
    }

    // Add devices below timestamp
    bool hasConnected = false;
    for (const auto& device : devices) {
        if (device->isConnected && device->batteryLevel.has_value()) {
            ss << L"\n" << device->name << L": " << device->batteryLevel.value() << L"%";
            hasConnected = true;
        }
    }

    if (!hasConnected) {
        ss << L"\nNo devices connected";
    }

    return ss.str();
}

std::wstring TrayApp::formatTimestamp(const SYSTEMTIME& time) {
    wchar_t buffer[64];
    swprintf_s(buffer, L"Updated: %02d:%02d:%02d",
               time.wHour, time.wMinute, time.wSecond);
    return std::wstring(buffer);
}

void TrayApp::startRefreshAnimation() {
    if (!isRefreshing) {
        isRefreshing = true;
        animationFrame = 0;
        SetTimer(hwnd, TIMER_REFRESH_ANIMATION, ANIMATION_INTERVAL, nullptr);

        // Update tooltip to show refreshing status
        wcscpy_s(notifyIconData.szTip, L"Refreshing...");
        Shell_NotifyIconW(NIM_MODIFY, &notifyIconData);
    }
}

void TrayApp::stopRefreshAnimation() {
    if (isRefreshing) {
        isRefreshing = false;
        animationFrame = 0;
        KillTimer(hwnd, TIMER_REFRESH_ANIMATION);
        KillTimer(hwnd, TIMER_ANIMATION_STOP);

        // Update icon with final battery data and tooltip
        updateTrayIcon();
    }
}

void TrayApp::updateRefreshAnimation() {
    if (!isRefreshing) return;

    // Create icon with spinning indicator
    // We'll add a small dot that rotates around the battery icon
    HICON currentIcon = batteryIcon->createBatteryIcon(std::nullopt);

    // Get device context for drawing
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);

    // Create bitmap for icon
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    // Draw current icon to bitmap
    DrawIconEx(memDC, 0, 0, currentIcon, 16, 16, 0, nullptr, DI_NORMAL);

    // Calculate spinner position (small circle rotating around center)
    double angle = (animationFrame * 2.0 * 3.14159) / ANIMATION_FRAMES;
    int centerX = 8;
    int centerY = 8;
    int radius = 6;
    int dotX = centerX + (int)(radius * cos(angle));
    int dotY = centerY + (int)(radius * sin(angle));

    // Draw small white dot as spinner indicator
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, whiteBrush);
    Ellipse(memDC, dotX - 1, dotY - 1, dotX + 2, dotY + 2);
    SelectObject(memDC, oldBrush);
    DeleteObject(whiteBrush);

    // Create icon from bitmap
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hBitmap;
    iconInfo.hbmMask = hBitmap;

    HICON animatedIcon = CreateIconIndirect(&iconInfo);

    // Update tray icon
    if (animatedIcon) {
        HICON oldIcon = notifyIconData.hIcon;
        notifyIconData.hIcon = animatedIcon;
        Shell_NotifyIconW(NIM_MODIFY, &notifyIconData);

        if (oldIcon != currentIcon) {
            DestroyIcon(oldIcon);
        }
    }

    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);
    DestroyIcon(currentIcon);

    // Advance to next frame
    animationFrame = (animationFrame + 1) % ANIMATION_FRAMES;
}

void TrayApp::run() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void TrayApp::cleanup() {
    if (hwnd) {
        KillTimer(hwnd, TIMER_REFRESH);
        KillTimer(hwnd, TIMER_REFRESH_ANIMATION);
        KillTimer(hwnd, TIMER_ANIMATION_STOP);
    }

    removeTrayIcon();

    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }

    UnregisterClassW(WINDOW_CLASS_NAME, hInstance);
}

LRESULT CALLBACK TrayApp::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Retrieve TrayApp instance from window user data
    TrayApp* app = nullptr;

    if (msg == WM_CREATE) {
        // Store 'this' pointer in window user data
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        app = reinterpret_cast<TrayApp*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    } else {
        app = reinterpret_cast<TrayApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (!app) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                app->showContextMenu();
            } else if (lParam == WM_LBUTTONDBLCLK) {
                app->refreshDevices();
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_MENU_REFRESH:
                    app->refreshDevices();
                    break;
                case ID_MENU_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_REFRESH) {
                app->refreshDevices();
            } else if (wParam == TIMER_REFRESH_ANIMATION) {
                app->updateRefreshAnimation();
            } else if (wParam == TIMER_ANIMATION_STOP) {
                app->stopRefreshAnimation();
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
