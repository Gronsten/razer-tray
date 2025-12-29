#pragma once
#include <string>
#include <vector>
#include <optional>
#include <windows.h>

struct DevicePattern {
    std::wstring name;
    std::wstring instanceIdPattern;
    bool enabled;
    std::wstring description;
};

struct Config {
    std::wstring version;
    std::vector<DevicePattern> devices;
    std::vector<std::wstring> namePatterns;
    int refreshInterval;

    struct BatteryThresholds {
        int high;
        int medium;
        int low;
    } batteryThresholds;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    // Load config from file (looks in executable directory by default)
    std::optional<Config> loadConfig(const std::wstring& configPath = L"");

    // Save config to file (saves to executable directory by default)
    bool saveConfig(const Config& config, const std::wstring& configPath = L"");

    // Get default config (Razer devices with standard settings)
    Config getDefaultConfig();

    // Get default config path (executable directory + config.json)
    std::wstring getDefaultConfigPath();

    // Check if device name matches any of the configured patterns
    bool matchesDevicePatterns(const std::wstring& deviceName, const Config& config);

private:
    // Parse JSON manually (simple implementation to avoid external dependencies)
    std::optional<Config> parseJson(const std::string& jsonContent);

    // Helper to read entire file
    std::optional<std::string> readFile(const std::wstring& path);

    // Helper to trim whitespace
    std::string trim(const std::string& str);

    // Helper to convert UTF-8 to wide string
    std::wstring utf8ToWide(const std::string& utf8);

    // Helper to convert wide string to UTF-8
    std::string wideToUtf8(const std::wstring& wide);

    // Helper to write file
    bool writeFile(const std::wstring& path, const std::string& content);

    // Helper to serialize config to JSON
    std::string serializeJson(const Config& config);
};
