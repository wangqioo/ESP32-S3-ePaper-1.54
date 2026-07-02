#ifndef DESKTOP_CONFIG_H
#define DESKTOP_CONFIG_H

namespace desktop_config {

inline constexpr const char *kWifiSsid = "";
inline constexpr const char *kWifiPassword = "";
inline constexpr const char *kLocationName = "SHANGHAI";
inline constexpr double kLatitude = 31.2304;
inline constexpr double kLongitude = 121.4737;
inline constexpr int kRefreshIntervalMinutes = 15;
inline constexpr int kWifiConnectTimeoutMs = 12000;
inline constexpr int kRefreshIntervalMs = kRefreshIntervalMinutes * 60 * 1000;
inline constexpr const char *kNtpServer = "ntp.aliyun.com";
inline constexpr int kNtpSyncTimeoutMs = 8000;
inline constexpr int kTimezoneOffsetSeconds = 8 * 60 * 60;

}  // namespace desktop_config

#endif
