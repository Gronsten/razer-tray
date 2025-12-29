#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
}

std::wstring ConfigManager::getDefaultConfigPath() {
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    std::wstring path(exePath);
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash + 1);
    }

    return path + L"config.json";
}

std::optional<std::string> ConfigManager::readFile(const std::wstring& path) {
    // Convert wstring to string for ifstream
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string ConfigManager::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\"");
    if (first == std::string::npos) return "";

    size_t last = str.find_last_not_of(" \t\n\r\"");
    return str.substr(first, (last - first + 1));
}

std::wstring ConfigManager::utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";

    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (size == 0) return L"";

    std::wstring result(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], size);

    return result;
}

std::string ConfigManager::wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";

    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size == 0) return "";

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], size, nullptr, nullptr);

    return result;
}

bool ConfigManager::writeFile(const std::wstring& path, const std::string& content) {
    std::ofstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }

    file << content;
    file.close();
    return true;
}

Config ConfigManager::getDefaultConfig() {
    Config config;
    config.version = L"1.0.0";
    config.devices = {};  // Empty - use patterns only
    config.namePatterns = {L"BSK*", L"Razer*"};
    config.refreshInterval = 300;  // 5 minutes
    config.batteryThresholds.high = 60;
    config.batteryThresholds.medium = 30;
    config.batteryThresholds.low = 15;
    return config;
}

std::string ConfigManager::serializeJson(const Config& config) {
    std::ostringstream json;

    json << "{\n";
    json << "  \"version\": \"" << wideToUtf8(config.version) << "\",\n";

    // Devices array
    json << "  \"devices\": [";
    for (size_t i = 0; i < config.devices.size(); i++) {
        const auto& device = config.devices[i];
        if (i > 0) json << ",";
        json << "\n    {\n";
        json << "      \"name\": \"" << wideToUtf8(device.name) << "\",\n";
        json << "      \"instanceIdPattern\": \"" << wideToUtf8(device.instanceIdPattern) << "\",\n";
        json << "      \"enabled\": " << (device.enabled ? "true" : "false") << ",\n";
        json << "      \"description\": \"" << wideToUtf8(device.description) << "\"\n";
        json << "    }";
    }
    if (config.devices.size() > 0) {
        json << "\n  ";
    }
    json << "],\n";

    // Name patterns array
    json << "  \"namePatterns\": [";
    for (size_t i = 0; i < config.namePatterns.size(); i++) {
        if (i > 0) json << ",";
        json << "\n    \"" << wideToUtf8(config.namePatterns[i]) << "\"";
    }
    if (config.namePatterns.size() > 0) {
        json << "\n  ";
    }
    json << "],\n";

    // Refresh interval
    json << "  \"refreshInterval\": " << config.refreshInterval << ",\n";

    // Battery thresholds
    json << "  \"batteryThresholds\": {\n";
    json << "    \"high\": " << config.batteryThresholds.high << ",\n";
    json << "    \"medium\": " << config.batteryThresholds.medium << ",\n";
    json << "    \"low\": " << config.batteryThresholds.low << "\n";
    json << "  }\n";

    json << "}\n";

    return json.str();
}

bool ConfigManager::saveConfig(const Config& config, const std::wstring& configPath) {
    std::wstring path = configPath.empty() ? getDefaultConfigPath() : configPath;

    std::string jsonContent = serializeJson(config);
    return writeFile(path, jsonContent);
}

std::optional<Config> ConfigManager::loadConfig(const std::wstring& configPath) {
    std::wstring path = configPath.empty() ? getDefaultConfigPath() : configPath;

    auto content = readFile(path);
    if (!content.has_value()) {
        return std::nullopt;
    }

    return parseJson(content.value());
}

// Simple JSON parser - extracts values between quotes
std::string extractQuotedValue(const std::string& json, const std::string& key) {
    std::string searchFor = "\"" + key + "\"";
    size_t pos = json.find(searchFor);
    if (pos == std::string::npos) return "";

    // Find the colon after the key
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";

    // Skip whitespace after colon
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    // If it's a quoted string
    if (json[pos] == '"') {
        pos++;
        size_t endPos = json.find('"', pos);
        if (endPos != std::string::npos) {
            return json.substr(pos, endPos - pos);
        }
    }

    return "";
}

int extractIntValue(const std::string& json, const std::string& key) {
    std::string searchFor = "\"" + key + "\"";
    size_t pos = json.find(searchFor);
    if (pos == std::string::npos) return 0;

    // Find the colon after the key
    pos = json.find(':', pos);
    if (pos == std::string::npos) return 0;

    // Skip whitespace after colon
    pos++;
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }

    // Extract digits
    std::string numStr;
    while (pos < json.length() && (json[pos] >= '0' && json[pos] <= '9')) {
        numStr += json[pos];
        pos++;
    }

    return numStr.empty() ? 0 : std::stoi(numStr);
}

std::vector<std::string> extractStringArray(const std::string& json, const std::string& arrayName) {
    std::vector<std::string> result;
    std::string searchFor = "\"" + arrayName + "\"";
    size_t pos = json.find(searchFor);
    if (pos == std::string::npos) return result;

    // Find the opening bracket [
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;

    // Find the closing bracket ]
    size_t endPos = json.find(']', pos);
    if (endPos == std::string::npos) return result;

    // Extract array content
    std::string arrayContent = json.substr(pos + 1, endPos - pos - 1);

    // Find all quoted strings
    pos = 0;
    while (true) {
        pos = arrayContent.find('"', pos);
        if (pos == std::string::npos) break;

        pos++; // Skip opening quote
        size_t endQuote = arrayContent.find('"', pos);
        if (endQuote == std::string::npos) break;

        result.push_back(arrayContent.substr(pos, endQuote - pos));
        pos = endQuote + 1;
    }

    return result;
}

std::optional<Config> ConfigManager::parseJson(const std::string& jsonContent) {
    Config config;

    try {
        // Parse simple values
        std::string version = extractQuotedValue(jsonContent, "version");
        config.version = utf8ToWide(version);

        config.refreshInterval = extractIntValue(jsonContent, "refreshInterval");
        if (config.refreshInterval == 0) {
            config.refreshInterval = 300; // default 5 minutes
        }

        // Parse battery thresholds
        size_t thresholdPos = jsonContent.find("\"batteryThresholds\"");
        if (thresholdPos != std::string::npos) {
            config.batteryThresholds.high = extractIntValue(jsonContent, "high");
            config.batteryThresholds.medium = extractIntValue(jsonContent, "medium");
            config.batteryThresholds.low = extractIntValue(jsonContent, "low");
        }

        // Set defaults if not found
        if (config.batteryThresholds.high == 0) config.batteryThresholds.high = 60;
        if (config.batteryThresholds.medium == 0) config.batteryThresholds.medium = 30;
        if (config.batteryThresholds.low == 0) config.batteryThresholds.low = 15;

        // Parse namePatterns array
        std::vector<std::string> patterns = extractStringArray(jsonContent, "namePatterns");
        for (const auto& pattern : patterns) {
            config.namePatterns.push_back(utf8ToWide(pattern));
        }

        // Parse devices array (simplified - just extract enabled device names)
        // This is a basic implementation - can be enhanced later
        size_t devicesPos = jsonContent.find("\"devices\"");
        if (devicesPos != std::string::npos) {
            size_t arrayStart = jsonContent.find('[', devicesPos);
            size_t arrayEnd = jsonContent.find(']', arrayStart);

            if (arrayStart != std::string::npos && arrayEnd != std::string::npos) {
                std::string devicesSection = jsonContent.substr(arrayStart, arrayEnd - arrayStart);

                // Find all device objects
                size_t pos = 0;
                while (true) {
                    pos = devicesSection.find('{', pos);
                    if (pos == std::string::npos) break;

                    size_t objEnd = devicesSection.find('}', pos);
                    if (objEnd == std::string::npos) break;

                    std::string deviceObj = devicesSection.substr(pos, objEnd - pos + 1);

                    DevicePattern device;
                    device.name = utf8ToWide(extractQuotedValue(deviceObj, "name"));
                    device.instanceIdPattern = utf8ToWide(extractQuotedValue(deviceObj, "instanceIdPattern"));
                    device.description = utf8ToWide(extractQuotedValue(deviceObj, "description"));

                    // Check enabled (default true)
                    size_t enabledPos = deviceObj.find("\"enabled\"");
                    if (enabledPos != std::string::npos) {
                        size_t truePos = deviceObj.find("true", enabledPos);
                        size_t falsePos = deviceObj.find("false", enabledPos);
                        device.enabled = (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos));
                    } else {
                        device.enabled = true;
                    }

                    if (!device.name.empty()) {
                        config.devices.push_back(device);
                    }

                    pos = objEnd + 1;
                }
            }
        }

        return config;

    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool ConfigManager::matchesDevicePatterns(const std::wstring& deviceName, const Config& config) {
    // Check against namePatterns (supports wildcards)
    for (const auto& pattern : config.namePatterns) {
        // Simple wildcard matching: * at end
        if (pattern.back() == L'*') {
            std::wstring prefix = pattern.substr(0, pattern.length() - 1);
            if (deviceName.find(prefix) == 0) {
                return true;
            }
        } else {
            // Exact match
            if (deviceName == pattern) {
                return true;
            }
        }
    }

    // Check against specific device names (if enabled)
    for (const auto& device : config.devices) {
        if (device.enabled && deviceName.find(device.name) != std::wstring::npos) {
            return true;
        }
    }

    return false;
}
