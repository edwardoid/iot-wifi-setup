#ifndef IOT_NM_UTILS_H
#define IOT_NM_UTILS_H

#include <NetworkManager.h>
#include "networksignals.h"

namespace IoT
{
    struct Utility
    {
        static Connection connectionFromNM(NMConnection* c, bool& ok);

        static WifiNetwork getWifiNetworkInfo(NMAccessPoint* ap, bool& ok);

        static WifiNetwork getCurrentNetwork(NMDeviceWifi* device, bool& ok);

        static ConnectionStatus deviceStateToConnectionStatus(NMDeviceState state);
    };
}

#endif // IOT_NM_UTILS_H