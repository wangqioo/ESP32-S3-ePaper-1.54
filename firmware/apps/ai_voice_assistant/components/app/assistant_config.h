#pragma once

#include <cstdint>

namespace assistant_config {

inline constexpr const char *kWifiSsid = "";
inline constexpr const char *kWifiPassword = "";
inline constexpr const char *kProxyHost = "";
inline constexpr uint16_t kProxyPort = 8787;
inline constexpr uint32_t kWifiConnectTimeoutMs = 12000;
inline constexpr uint32_t kUploadTimeoutMs = 30000;
inline constexpr const char *kAskPath = "/ask";

inline bool WifiConfigured() {
    return kWifiSsid != nullptr && kWifiSsid[0] != '\0';
}

inline bool ProxyConfigured() {
    return kProxyHost != nullptr && kProxyHost[0] != '\0';
}

}  // namespace assistant_config
