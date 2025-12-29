#include <windows.h>
#include "TrayApp.h"

// WinMain - Windows GUI application entry point
// MinGW uses WinMain, not wWinMain
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // Unreferenced parameters
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Create and initialize the tray application
    TrayApp app(hInstance);

    if (!app.initialize()) {
        MessageBoxW(
            nullptr,
            L"Failed to initialize Razer Battery Tray Monitor.",
            L"Initialization Error",
            MB_ICONERROR | MB_OK
        );
        return 1;
    }

    // Run the message loop
    app.run();

    // Cleanup is handled by TrayApp destructor
    return 0;
}
