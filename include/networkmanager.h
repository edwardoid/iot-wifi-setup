#ifndef IOT_NETWORK_MANAGER_H
#define IOT_NETWORK_MANAGER_H

#include <vector>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <NetworkManager.h>
#include "networksignals.h"

using namespace SignalSlot;

namespace IoT
{
    class NetworkManager: public NetworkSignals
    {
        NetworkManager();
        ~NetworkManager();
    public:
        std::vector<std::string> devices();
        std::vector<WifiNetwork> scan(std::string interface, bool force = false);
        std::vector<Connection> connections();
        Connection activeConnection(std::string interface);
        WifiNetwork activeNetwork(std::string interface);
        bool activateConnection(std::string uuid);
        Result connectoToNetwork(std::string iface, WifiNetwork wifi);
        Result createHotspot(std::string iface, WifiNetwork wifi);
        static NetworkManager& i();
        void update();

        SignalSlot::Signal<std::string, ActiveConnection> ActiveConnectionChanged;
    private:
        NMAccessPoint* getAccessPoint(std::string iface, std::string ssid);
    protected:
        struct Data* m_data;
        std::unordered_map<std::string, ActiveConnection> m_activeConnections;
        std::thread m_networkTracker;
    };
}

#endif // IOT_NETWORK_MANAGER_H