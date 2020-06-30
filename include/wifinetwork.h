#ifndef IOT_WIFI_NETWORK_H
#define IOT_WIFI_NETWORK_H

#include <string>

namespace IoT
{
    enum class Authentication
    {
        None,
        WEP,
        WPA,
        WPA2,
        Enterprise
    };

    struct WifiNetwork
    {
        std::string ssid;
        std::string password;
        Authentication auth;
        bool encrypted = 0;
        int signal = 100;
        const bool operator == (const WifiNetwork& wifi)
        {
            return ssid == wifi.ssid && auth == wifi.auth && encrypted == wifi.encrypted;
        }
    };
}

#endif // IOT_WIFI_NETWORK_H