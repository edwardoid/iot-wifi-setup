#ifndef IOT_SIGNALS_H
#define IOT_SIGNALS_H

#include <s2s.h>
#include <s2s_property.h>
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
        bool encrypted;
        int signal;
        const bool operator == (const WifiNetwork& wifi)
        {
            return ssid == wifi.ssid && auth == wifi.auth && encrypted == wifi.encrypted;
        }
    };

    enum class NetworkState
    {
        UnknownYet = 0,
        Disconnected,
        Limited,
        Connected
    };

    enum class Mode {
        AdHoc,
        AccessPoint,
        Infrastructure
    };

    enum class ConnectionStatus
    {
        Disconnected,
        Connecting,
        Connected
    };

    struct Connection
    {
        Mode mode;
        std::string uuid;
        std::string name;
    };

    struct NetworkSignals
    {
        SignalSlot::Signal<Connection> ConnectionAdded;
        SignalSlot::Signal<Connection> ConnectionActivated;
        SignalSlot::Signal<bool> InternetConenctionAvailable;
        SignalSlot::Signal<std::string, WifiNetwork> DeviceWiFiNetworkChanged;
        SignalSlot::Signal<Connection, ConnectionStatus> ConnectionChanged;
        SignalSlot::Property<NetworkState> Connectivity;
    };
}

#endif // IOT_SIGNALS_H
