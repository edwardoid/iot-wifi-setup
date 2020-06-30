#ifndef IOT_NETWORK_MANAGER_H
#define IOT_NETWORK_MANAGER_H

#include <vector>
#include <string>
#include <thread>
#include <functional>
#include <NetworkManager.h>
#include "signals.h"

using namespace SignalSlot;

namespace IoT
{
    class NetworkManager: public NetworkSignals
    {
        NetworkManager();
        ~NetworkManager();
    public:
        std::vector<std::string> devices();
        std::vector<WifiNetwork> scan(std::string interface);
        std::vector<Connection> connections();
        bool activateConnection(std::string uuid);
        bool connectoToNetwork(std::string iface, WifiNetwork wifi);
        bool createHotspot(std::string iface, WifiNetwork wifi);
        bool getDetails(std::string interface);
        void trackConnection(std::string uuid);
        static NetworkManager& i();
        void update();
    private:
        NMAccessPoint* getAccessPoint(std::string iface, std::string ssid);
    protected:
        struct Data* m_data;
        std::thread m_routine;
    };
}

#endif // IOT_NETWORK_MANAGER_H