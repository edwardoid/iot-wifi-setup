#ifndef IOT_WIFI_SETUP_H
#define IOT_WIFI_SETUP_H

#include <string>
#include <memory>
#include <vector>

namespace IoT
{
    class WiFi
    {
    public:

        enum State
        {
            Uninitialized,
            CheckingConnectivity,
            Disconnected,
            SwitchingToAP,
            InAPMode,
            TryingToConnect,
            Connected
        };

        void init(std::string iface,
                  std::string apSSID,
                  std::string apPassword,
                  bool autoSwitchInAPMode = true);

        void switchToAPMode();

        void start();

        std::vector<std::string> availableNetworks();

        void tryConnect(std::string ssid,
                        std::string password);

        void onStateChanged(std::function<void(State)> state);

        State state() const;
        std::string currentSSID();
        std::string currentIP();
    private:
        void stateChanged(State newState);
        void findAPConnection();
    private:
        std::function<void(State)> m_onStateChanged;
        std::string m_iface;
        std::string m_apSSID;
        std::string m_apPassword;
        std::string m_apConnectionID;
        std::string m_internetConnectionID;

        std::string m_currentSSID;
        std::string m_currentIP;
        bool m_autoSwitchInAPMode;
        State m_state;
    };
}

#endif // IOT_WIFI_SETUP_H
