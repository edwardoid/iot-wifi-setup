#include "wifi_setup.h"
#include "log.h"
#include "networkmanager.h"
#include <algorithm>

using namespace IoT;

void WiFi::init(std::string iface, std::string apSSID, std::string apPassword, bool autoSwitchInAPMode)
{
    m_apSSID = apSSID;
    m_apPassword = apPassword;

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
    NetworkManager::i().InternetConnectionAvailable.connect([this](const bool& ok) {
        if (state() == State::TryingToConnect || state() == State::Connected || state() == State::CheckingConnectivity) {
            if (ok) {
                stateChanged(State::Connected);
            } else {
                switchToAPMode();
            }
        }
    });

    NetworkManager::i().ActiveConnectionChanged.connect([this](std::string iface, ActiveConnection connection){
        if (iface != m_iface) {
            return;
        }
        m_currentSSID = connection.ssid;
        m_currentIP = connection.ip;
        m_signal = connection.signal;
    });

    NetworkManager::i().update();
}

void WiFi::updateInternetConnectivity(bool conencted)
{
    NetworkManager::i().InternetConnectionAvailable.set(conencted);
}

void WiFi::switchToAPMode()
{
    if (m_apConnectionID.empty()) {
        LOG_ERROR << "No AP mode specified, can't switch to AP";
        stateChanged(IoT::WiFi::Uninitialized);
        return;
    }

    stateChanged(State::SwitchingToAP);

    if(!NetworkManager::i().activateConnection(m_apConnectionID) || NetworkManager::i().LastConnectResult.value != Result::Connected) {
        LOG_ERROR << "Can't activate connection " << m_apConnectionID;
        stateChanged(IoT::WiFi::Uninitialized);
        return;
    }
    stateChanged(State::InAPMode);
}

void WiFi::findAPConnection()
{
    for(const IoT::Connection& c : NetworkManager::i().connections()) {
        if (c.mode != IoT::Mode::AccessPoint || c.name != m_apSSID) {
            continue;
        }

        m_apConnectionID = c.uuid;
        break;
    }

    if (m_apConnectionID.empty()) {
        LOG_WARN << "There is no AP mode connection, creating one";
        WifiNetwork net;
        net.ssid = m_apSSID;
        net.password = m_apPassword;
        net.auth = IoT::Authentication::WPA2;
        if(NetworkManager::i().createHotspot(m_iface, net) != Result::Added) {
            LOG_ERROR << "Can't create access point connection";
            stateChanged(State::Uninitialized);
            return;
        }
    }

    for(const IoT::Connection& c : NetworkManager::i().connections()) {
        if (c.mode != IoT::Mode::AccessPoint || c.name != m_apSSID) {
            continue;
        }

        m_apConnectionID = c.uuid;
        break;
    }

    LOG_DEBUG << "AP mode connection: " << m_apConnectionID;
    if (!NetworkManager::i().InternetConnectionAvailable.value) {
        switchToAPMode();
    }
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

std::vector<WifiNetwork> WiFi::availableNetworks(bool scan)
{
    std::vector<WifiNetwork> networks = NetworkManager::i().scan(m_iface, scan);
    std::sort(networks.begin(), networks.end(), [](const WifiNetwork& l, const WifiNetwork& r) { return l.signal > r.signal; });
    return networks;
}

void WiFi::tryConnect(std::string ssid, std::string password)
{
    stateChanged(State::TryingToConnect);
    WifiNetwork net;
    net.ssid = ssid;
    net.password = password;
    net.auth = Authentication::WPA2;
    if(Result::Connected != NetworkManager::i().connectoToNetwork(m_iface, net))
    {
        stateChanged(State::Disconnected);
        switchToAPMode();
    }
}

std::string WiFi::currentSSID() const
{
    return m_currentSSID;
}

int WiFi::wifiSignal() const
{
    return m_signal;
}


std::string WiFi::currentIP() const
{
    return m_currentIP;
}
