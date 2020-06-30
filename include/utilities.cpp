#include "utilities.h"
#include "log.h"
using namespace IoT;


Connection Utility::connectionFromNM(NMConnection* c, bool& ok)
{
    Connection conn;
    if (c == NULL) {
        LOG_ERROR << "Connection object is NULL";
        ok = false;
        return conn;
    }

    NMSettingWireless *s = nm_connection_get_setting_wireless(c);
    if (s == NULL) {
        ok = false;
        return conn;
    }

    conn.uuid = nm_connection_get_uuid(c);

    GBytes *ssid = nm_setting_wireless_get_ssid(s);
    if (ssid != NULL) {
        conn.name = std::string((const char *)g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
    } else {
        LOG_ERROR << "SSID it not available";
        ok = false;
        
        return conn;
    }
    std::string mode = nm_setting_wireless_get_mode(s);
    if (mode == NM_SETTING_WIRELESS_MODE_AP) {
        conn.mode = Mode::AccessPoint;
    } else if (mode == NM_SETTING_WIRELESS_MODE_INFRA) {
        conn.mode = Mode::Infrastructure;
    } else if (mode == NM_SETTING_WIRELESS_MODE_ADHOC) {
        conn.mode = Mode::AdHoc;
    } else {
        LOG_ERROR << "Unknown mode: " << mode;
        ok = false;
        return conn;
    }

    return conn;
}

WifiNetwork Utility::getWifiNetworkInfo(NMAccessPoint* ap, bool& ok)
{
    ok = false;
    WifiNetwork wifi;
    GBytes *ssid = nm_access_point_get_ssid(ap);
    if (ssid == NULL) {
        return wifi;
    }
    wifi.ssid = std::string((const char *)g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid));
    wifi.signal = nm_access_point_get_strength(ap);

    auto flags = nm_access_point_get_flags(ap);
    auto wpa_flags = nm_access_point_get_wpa_flags(ap);
    auto rsn_flags = nm_access_point_get_rsn_flags(ap);

    wifi.encrypted = !(flags & NM_802_11_AP_FLAGS_PRIVACY) && (wpa_flags != NM_802_11_AP_SEC_NONE) && (rsn_flags != NM_802_11_AP_SEC_NONE);

    wifi.auth = Authentication::None;
    if ((flags & NM_802_11_AP_FLAGS_PRIVACY) && (wpa_flags == NM_802_11_AP_SEC_NONE) && (rsn_flags == NM_802_11_AP_SEC_NONE)) {
        wifi.auth = Authentication::WEP;
    }

    if (wpa_flags != NM_802_11_AP_SEC_NONE) {
        wifi.auth = Authentication::WPA;
    }

    if (rsn_flags != NM_802_11_AP_SEC_NONE) {
        wifi.auth = Authentication::WPA2;
    }

    if ((wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X) || (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)) {
        wifi.auth = Authentication::Enterprise;
    }
    ok = true;
    return wifi;
}

WifiNetwork Utility::getCurrentNetwork(NMDeviceWifi* device, bool& ok)
{
    WifiNetwork wifi;
    NMAccessPoint* ap = nm_device_wifi_get_active_access_point(device);
    if (ap == NULL) {
        ok = false;
        return wifi;
    }
    return getWifiNetworkInfo(ap, ok);
}

ConnectionStatus Utility::deviceStateToConnectionStatus(NMDeviceState state)
{
    switch(state) {
        case NMDeviceState::NM_DEVICE_STATE_ACTIVATED: {
            return ConnectionStatus::Connected;
            break;
        }
        case NMDeviceState::NM_DEVICE_STATE_PREPARE:
        case NMDeviceState::NM_DEVICE_STATE_CONFIG:
        case NMDeviceState::NM_DEVICE_STATE_NEED_AUTH:
        case NMDeviceState::NM_DEVICE_STATE_IP_CONFIG: {
            return ConnectionStatus::Connecting;
            break;
        }
        default: {
            return ConnectionStatus::Disconnected;
            break;
        }
    }
}