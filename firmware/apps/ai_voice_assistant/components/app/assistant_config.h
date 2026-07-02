#pragma once

#include <cstdint>

#if __has_include("assistant_config_local.h")
#include "assistant_config_local.h"
#endif

#ifndef ASSISTANT_WIFI_SSID
#define ASSISTANT_WIFI_SSID ""
#endif

#ifndef ASSISTANT_WIFI_PASSWORD
#define ASSISTANT_WIFI_PASSWORD ""
#endif

#ifndef ASSISTANT_PROXY_HOST
#define ASSISTANT_PROXY_HOST ""
#endif

#ifndef ASSISTANT_PROXY_PORT
#define ASSISTANT_PROXY_PORT 8787
#endif

namespace assistant_config {

inline constexpr const char *kWifiSsid = ASSISTANT_WIFI_SSID;
inline constexpr const char *kWifiPassword = ASSISTANT_WIFI_PASSWORD;
inline constexpr const char *kProxyHost = ASSISTANT_PROXY_HOST;
inline constexpr uint16_t kProxyPort = ASSISTANT_PROXY_PORT;
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
