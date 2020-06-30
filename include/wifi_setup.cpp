#include "wifi_setup.h"
#include "log.h"
#include "networkmanager.h"
#include <iostream>

using namespace IoT;

void WiFi::init(std::string iface, std::string apSSID, std::string apPassword, bool autoSwitchInAPMode)
{
    m_apSSID = apSSID;
    m_apPassword = apPassword;
    m_autoSwitchInAPMode = autoSwitchInAPMode;

    NetworkManager::i().ConnectionAdded.connect([this](Connection c) {
        if (c.mode == Mode::AccessPoint) {
            m_apConnectionID = c.uuid;
        }
    });

    NetworkManager::i().ConnectionActivated.connect([this](Connection c) {
        if (c.uuid == m_apConnectionID) {
            stateChanged(State::InAPMode);
        }
    });

    NetworkManager::i().Connectivity.connect([this](const NetworkState& state) {
        switch(state) {
            case NetworkState::Disconnected: {
                LOG_WARN << "Connection lost. Waiting for updates from device(if not received yet)";
                stateChanged(State::Disconnected);
                switchToAPMode();
                break;
            }
            case NetworkState::Limited: {
                LOG_WARN << "Connection is limited";
                break;
            }
            case NetworkState::Connected: {
                if (m_currentSSID != m_apSSID) {
                    stateChanged(State::Connected);
                }
                break;
            }
            case NetworkState::UnknownYet:
            default:  {

            }
        }
    });

    NetworkManager::i().ConnectionChanged.connect([this](Connection c, ConnectionStatus connected) {
        if (m_apConnectionID == c.uuid) {
            switch(connected) {
                case ConnectionStatus::Connecting: {
                    stateChanged(State::SwitchingToAP);
                    break;
                }
                case ConnectionStatus::Connected: {
                    stateChanged(State::InAPMode);
                    break;
                }
                case ConnectionStatus::Disconnected:
                default: {}
            }
        } else {
            switch(connected) {
                case ConnectionStatus::Connecting: {
                    stateChanged(State::TryingToConnect);
                    break;
                }
                case ConnectionStatus::Connected: {
                    stateChanged(State::CheckingConnectivity);
                    break;
                }
                case ConnectionStatus::Disconnected: {
                    stateChanged(State::Disconnected);
                    switchToAPMode();
                    break;
                }
                default: {}
            }
        }
    });

    NetworkManager::i().DeviceWiFiNetworkChanged.connect([this](std::string iface, WifiNetwork network) {
        if (iface == m_iface) {
            m_currentSSID = network.ssid;
        }
    });

    m_iface = iface;
    if (m_iface.empty()) {
        auto allDevs = NetworkManager::i().devices();
        if (allDevs.empty()) {
            LOG_ERROR << "No WiFi device available in the system";
            stateChanged(State::Uninitialized);
            return;
        }
        m_iface = allDevs[0];
        LOG_DEBUG << "Selected " << m_iface << " as wifi device!";
    }
    stateChanged(State::CheckingConnectivity);
    findAPConnection();

    NetworkManager::i().update();
}

void WiFi::switchToAPMode()
{
    if (m_apConnectionID.empty()) {
        LOG_ERROR << "No AP mode specified, can't switch to AP";
    }

    if(!NetworkManager::i().activateConnection(m_apConnectionID)) {
        LOG_ERROR << "Can't activate connection " << m_apConnectionID;
    }
    stateChanged(State::SwitchingToAP);
}

void WiFi::findAPConnection()
{
    for(const IoT::Connection& c : NetworkManager::i().connections())
    {
        if (c.mode != IoT::Mode::AccessPoint) {
            continue;
        }

        if (c.name != m_apSSID) {
            continue;
        }

        m_apConnectionID = c.uuid;
    }

    if (m_apConnectionID.empty()) {
        LOG_WARN << "There is no AP mode connection, creating one";
        WifiNetwork net;
        net.ssid = m_apSSID;
        net.password = m_apPassword;
        net.auth = IoT::Authentication::WPA2;
        if(!NetworkManager::i().createHotspot(m_iface, net)) {
            LOG_ERROR << "Can't create access point connection";
            stateChanged(State::Uninitialized);
            return;
        }
    }

    LOG_DEBUG << "AP mode connection: " << m_apConnectionID;
}

void WiFi::onStateChanged(std::function<void(WiFi::State)> cb) {
    m_onStateChanged = std::move(cb);
}

WiFi::State WiFi::state() const
{
    return m_state;
}

void WiFi::stateChanged(State newState)
{
    m_state = newState;
    if(newState == State::Uninitialized) LOG_DEBUG << "State == " << "State::Uninitialized";
    if(newState == State::CheckingConnectivity) LOG_DEBUG << "State == " << "State::CheckingConnectivity";
    if(newState == State::Disconnected) LOG_DEBUG << "State == " << "State::Disconnected";
    if(newState == State::SwitchingToAP) LOG_DEBUG << "State == " << "State::SwitchingToAP";
    if(newState == State::InAPMode) LOG_DEBUG << "State == " << "State::InAPMode";
    if(newState == State::TryingToConnect) LOG_DEBUG << "State == " << "State::TryingToConnect";
    if(newState == State::Connected) LOG_DEBUG << "State == " << "State::Connected";
    m_onStateChanged(m_state);
}

std::vector<std::string> WiFi::availableNetworks()
{
    
}

void WiFi::tryConnect(std::string ssid, std::string password)
{
    WifiNetwork net;
    net.ssid = ssid;
    net.password = password;
    net.auth = Authentication::WPA2;
    NetworkManager::i().connectoToNetwork(m_iface, net);
}

void WiFi::start()
{

}

std::string WiFi::currentSSID()
{
    return m_currentSSID;
}

std::string WiFi::currentIP()
{
    return m_currentIP;
}

