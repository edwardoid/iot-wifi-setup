#ifndef IOT_SIGNALS_H
#define IOT_SIGNALS_H

#include "wifinetwork.h"
#include <s2s.h>
#include <s2s_property.h>
#include <string>

namespace IoT
{
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
        std::string ip;
    };

    enum class Result
    {
        Unknown = 0,
        Initilizaling,
        InternalError,
        InterfaceNotFound,
        BadParameters,
        NetworkNotFound,
        BadCredentials,
        Added,
        Connected,
        Disconnected
    };

    struct ActiveConnection: public Connection, public WifiNetwork
    {
        bool operator == (const ActiveConnection& src) const
        {
            return ssid == src.ssid &&
                   auth == src.auth &&
                   encrypted == src.encrypted &&
                   signal == src.signal &&
                   mode == src.mode &&
                   uuid == src.uuid &&
                   name == src.name &&
                   ip == src.ip;
        }

        bool operator != (const ActiveConnection& src) const
        {
            return !(*this == src);
        }
    };

    struct NetworkSignals
    {
        SignalSlot::Property<bool> InternetConnectionAvailable;
        SignalSlot::Property<Result> LastConnectResult;
    };
}

#endif // IOT_SIGNALS_H
