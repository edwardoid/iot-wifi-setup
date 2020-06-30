#include "networkmanager.h"
#include <glib.h>
#include "nm_private_data.h"
#include "log.h"
#include "utilities.h"
#include "callbacks.h"
#include <string.h>
#include <thread>
#include <chrono>

using namespace IoT;

NetworkManager::NetworkManager()
    : m_data(new Data())
{
    LOG_DEBUG << "Creating NetworkManager";
    m_data->Client = nm_client_new(NULL, NULL);
    if (m_data->Client != nullptr)
    {
        LOG_DEBUG << "Connected to NetworkManager version: " << nm_client_get_version(m_data->Client);
    }
    else
    {
        LOG_ERROR << "Can't connect to NetworkManager";
    }

    m_data->Loop = g_main_loop_new(NULL, FALSE);

    if (!nm_client_wireless_get_enabled(m_data->Client))
        nm_client_wireless_set_enabled(m_data->Client, TRUE);

    InternetConnectionAvailable.bind(m_data->InternetConnectionAvailable);
    LastConnectResult.bind(m_data->LastConnectResult);

    m_networkTracker = std::thread([this]() {
        const GPtrArray *devicesArr = nm_client_get_devices(m_data->Client);
        while(1) {
            while (g_main_context_iteration(NULL, FALSE) == TRUE) {
            }
            for (int i = 0; i < devicesArr->len; i++)
            {
                NMDevice *device = NM_DEVICE(g_ptr_array_index(devicesArr, i));
                if (!NM_IS_DEVICE_WIFI(device))
                {
                    continue;
                }

                std::string iface = nm_device_get_iface(device);
                auto state = nm_device_get_state(device);
                ConnectionStatus now = Utility::deviceStateToConnectionStatus(state);

                switch (now)
                {
                    case ConnectionStatus::Connected:
                    {
                        m_data->InternetConnectionAvailable = true;
                        break;
                    }
                    case ConnectionStatus::Disconnected:
                    {
                        m_data->InternetConnectionAvailable = false;
                        break;
                    }
                }

                ActiveConnection connection;
                static_cast<Connection&>(connection) = activeConnection(iface);
                static_cast<WifiNetwork&>(connection) = activeNetwork(iface);
                auto pos = m_activeConnections.find(iface);
                if (pos == m_activeConnections.end()) {
                    m_activeConnections.insert({iface, connection});
                    ActiveConnectionChanged.emit(iface, connection);
                }
                else if (pos->second != connection) {
                    pos->second = connection;
                    ActiveConnectionChanged.emit(iface, connection);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    });
}

void NetworkManager::update()
{
    InternetConnectionAvailable.emit(InternetConnectionAvailable.value);
}

NetworkManager::~NetworkManager()
{
    if (m_data->Client)
    {
        g_object_unref(m_data->Client);
    }

    if (m_data->Loop)
    {
        g_main_loop_unref(m_data->Loop);
    }

    delete m_data;
}

std::vector<std::string> NetworkManager::devices()
{
    std::vector<std::string> devs;
    if (m_data->Client == nullptr)
    {
        LOG_ERROR << "Not connected to Network Manager";
        return devs;
    }

    const GPtrArray *devicesArr = nm_client_get_devices(m_data->Client);
    for (int i = 0; i < devicesArr->len; i++)
    {
        NMDevice *device = NM_DEVICE(g_ptr_array_index(devicesArr, i));
        if (NM_IS_DEVICE_WIFI(device))
        {
            devs.emplace_back(nm_device_get_iface(device));
        }
    }

    return devs;
}

NMAccessPoint *NetworkManager::getAccessPoint(std::string iface, std::string SSID)
{
    NMDevice *dev = nm_client_get_device_by_iface(m_data->Client, iface.c_str());
    if (dev == NULL)
    {
        LOG_ERROR << "Can't find device " << iface;
        return NULL;
    }

    if (!NM_IS_DEVICE_WIFI(dev))
    {
        LOG_ERROR << "Interface " << iface << " is not WiFi device";
        return NULL;
    }

    GError *err = NULL;
    if (!nm_device_wifi_request_scan(NM_DEVICE_WIFI(dev), NULL, &err) && err->code != 6)
    {
        LOG_ERROR << "Can't perform scan Error:" << err->code << " " << err->message;
        g_error_free(err);
        return NULL;
    }

    if (err == NULL)
        std::this_thread::sleep_for(std::chrono::seconds(5));
    else
    {
        g_error_free(err);
    }

    const GPtrArray *aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(dev));

    for (int i = 0; i < aps->len; i++)
    {
        NMAccessPoint *ap = NM_ACCESS_POINT(g_ptr_array_index(aps, i));
        if (!ap)
        {
            continue;
        }

        GBytes *ssid = nm_access_point_get_ssid(ap);
        if (ssid == NULL)
            continue;
        if (SSID == std::string((const char *)g_bytes_get_data(ssid, NULL), g_bytes_get_size(ssid)))
            return ap;
    }
    return NULL;
}

std::vector<WifiNetwork> NetworkManager::scan(std::string interface, bool force)
{
    std::vector<WifiNetwork> nets;
    NMDevice *dev = nm_client_get_device_by_iface(m_data->Client, interface.c_str());
    if (dev == NULL)
    {
        LOG_ERROR << "Can't find device " << interface;
        return nets;
    }

    if (!NM_IS_DEVICE_WIFI(dev))
    {
        LOG_ERROR << "Interface " << interface << " is not WiFi device";
        return nets;
    }

    WifiScanData data;
    data.force = force;
    std::unique_lock<std::mutex> lock(data.mx);
    nm_device_wifi_request_scan_async(NM_DEVICE_WIFI(dev), NULL, Callbacks::scanCompleted, &data);

    data.cv.wait(lock);

    const GPtrArray *aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI(dev));

    for (int i = 0; i < aps->len; i++)
    {
        NMAccessPoint *ap = NM_ACCESS_POINT(g_ptr_array_index(aps, i));
        if (!ap)
        {
            continue;
        }

        bool ok = false;
        WifiNetwork wifi = Utility::getWifiNetworkInfo(ap, ok);
        if (!ok)
        {
            continue;
        }
        nets.push_back(wifi);
    }
    return nets;
}

std::vector<Connection> NetworkManager::connections()
{
    std::vector<Connection> conns;
    const GPtrArray *available = nm_client_get_connections(m_data->Client);
    for (int i = 0; i < available->len; ++i)
    {
        NMConnection *c = NM_CONNECTION(g_ptr_array_index(available, i));

        bool ok = true;
        Connection conn = Utility::connectionFromNM(c, ok);
        if (ok)
        {
            conns.push_back(conn);
        }
    }

    return conns;
}

Connection NetworkManager::activeConnection(std::string interface)
{
    Connection c;
    NMDevice *device = nm_client_get_device_by_iface(m_data->Client, interface.c_str());
    if (!NM_IS_DEVICE_WIFI(device)) {
        return c;
    }
    NMActiveConnection* connection = nm_device_get_active_connection(device);

    if (connection) {
        NMRemoteConnection* remote = nm_active_connection_get_connection(connection);
        bool ok;
        c = Utility::connectionFromNM(NM_CONNECTION(remote), ok);
        NMIPConfig* cfg = nm_active_connection_get_ip4_config(connection);
        if (cfg) {
            const GPtrArray *ips = nm_ip_config_get_addresses(cfg);
            for (int i = 0; i < ips->len; i++) {
                NMIPAddress *ip = (NMIPAddress*)(g_ptr_array_index(ips, i));
                if (ip != NULL) {
                    c.ip = nm_ip_address_get_address(ip);
                }

                if (!c.ip.empty()) {
                    break;
                }
            }
        }
    }

    return c;
}

WifiNetwork NetworkManager::activeNetwork(std::string interface)
{
    WifiNetwork network;
    NMDevice *device = nm_client_get_device_by_iface(m_data->Client, interface.c_str());
    if (!NM_IS_DEVICE_WIFI(device)) {
        return network;
    }

    bool ok = false;
    return Utility::getCurrentNetwork(NM_DEVICE_WIFI(device), ok);
}

bool NetworkManager::activateConnection(std::string uuid)
{
    NMRemoteConnection *conn = nm_client_get_connection_by_uuid(m_data->Client, uuid.c_str());
    if (conn == NULL)
    {
        return false;
    }

    AddConnectionData *data = new AddConnectionData;
    data->data = m_data;
    data->Activate = true;

    nm_client_activate_connection_async(m_data->Client, NM_CONNECTION(conn),
                                        NULL, NULL, NULL,
                                        Callbacks::connectionActivated, data);
    g_main_loop_run(m_data->Loop); // Wait
    return true;
}

Result NetworkManager::connectoToNetwork(std::string iface, WifiNetwork network)
{
    InternetConnectionAvailable.blockSignals(true);
    LastConnectResult = Result::Initilizaling;

    auto ctx = g_main_loop_get_context(m_data->Loop);
    int count = 0;
    NMDevice *device = nm_client_get_device_by_iface(m_data->Client, iface.c_str());
    if (!NM_IS_DEVICE_WIFI(device))
    {
        InternetConnectionAvailable.blockSignals(false);
        return LastConnectResult.set(Result::InterfaceNotFound);
    }

    GError *error = NULL;

    NMActiveConnection *activeConnection = nm_device_get_active_connection(device);
    if (activeConnection != NULL)
    {
        NMRemoteConnection *remote = nm_active_connection_get_connection(activeConnection);
        if (remote == NULL)
        {
            InternetConnectionAvailable.blockSignals(false);
            return LastConnectResult.set(Result::InternalError);
        }

        NMSettingWireless *s = nm_connection_get_setting_wireless(NM_CONNECTION(remote));
        if (s == NULL)
        {
            InternetConnectionAvailable.blockSignals(false);
            return LastConnectResult.set(Result::InternalError);
        }

        std::string mode = nm_setting_wireless_get_mode(s);

        if (mode == NM_SETTING_WIRELESS_MODE_AP)
        {
            LOG_DEBUG << "Current connection is HotSpot and scanning is not available. Deactivateing to scan";
            if (!nm_client_deactivate_connection(m_data->Client, activeConnection, NULL, &error) || error != NULL)
            {
                LOG_DEBUG << "Can't deactivate active conenction. Error:" << error->message;
                g_error_free(error);
                InternetConnectionAvailable.blockSignals(false);
                return LastConnectResult.set(Result::InternalError);
            }

            nm_client_wireless_set_enabled(m_data->Client, FALSE);
            nm_client_wireless_set_enabled(m_data->Client, TRUE);
        }
    }

    NMAccessPoint *ap = NULL;
    while (count < 45 && ap == NULL)
    {
        LOG_DEBUG << "Trying to find network " << network.ssid;
        while (g_main_context_pending(NULL)) {
            g_main_context_iteration(NULL, FALSE);
        }
        ap = getAccessPoint(iface, network.ssid);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        count++;
    }

    if (ap == NULL)
    {
        LOG_ERROR << "Network not found";
        InternetConnectionAvailable.blockSignals(false);
        return LastConnectResult.set(Result::NetworkNotFound);
    }

    LOG_DEBUG << "Network found, connecting...";

    NMConnection *connection = nm_simple_connection_new();

    NMSettingConnection *settingConnection = (NMSettingConnection *)nm_setting_connection_new();
    char *uuid = nm_utils_uuid_generate();
    g_object_set(G_OBJECT(settingConnection),
                 NM_SETTING_CONNECTION_UUID, uuid,
                 NM_SETTING_CONNECTION_ID, network.ssid.c_str(),
                 NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                 NM_SETTING_CONNECTION_AUTOCONNECT, TRUE,
                 NULL);
    g_free(uuid);
    nm_connection_add_setting(connection, NM_SETTING(settingConnection));

    auto bssid = nm_access_point_get_bssid(ap);

    /* Build up the 'wired' Setting */
    NMSettingWireless *wireless = (NMSettingWireless *)nm_setting_wireless_new();
    g_object_set(G_OBJECT(wireless),
                 NM_SETTING_WIRELESS_SSID, nm_access_point_get_ssid(ap),
                 NM_SETTING_WIRELESS_BSSID, nm_access_point_get_bssid(ap),
                 NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_INFRA,
                 NM_SETTING_WIRELESS_BAND, "bg",
                 NULL);

    nm_connection_add_setting(connection, NM_SETTING(wireless));

    if (network.auth == Authentication::Enterprise)
    {
        LOG_ERROR << "Enterprize networks are not supported";
        InternetConnectionAvailable.blockSignals(false);
        return LastConnectResult.set(Result::BadParameters);
    }

    NMSettingWirelessSecurity *security = NULL;
    if (network.auth == Authentication::WEP)
    {
        security = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
        g_object_set(G_OBJECT(security),
                     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                     NM_SETTING_WIRELESS_SECURITY_WEP_TX_KEYIDX, 0,
                     NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                     NM_SETTING_WIRELESS_SECURITY_WEP_KEY0, network.password.c_str(),
                     NULL);
        if (network.password.size() == 32)
        {
            g_object_set(G_OBJECT(security),
                         NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                         NULL);
        }
    }
    else if (network.auth == Authentication::WPA || network.auth == Authentication::WPA2)
    {
        security = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
        g_object_set(G_OBJECT(security),
                     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                     NM_SETTING_WIRELESS_SECURITY_PSK, network.password.c_str(),
                     NULL);
    }

    if (security != NULL) {
        nm_connection_add_setting(connection, NM_SETTING(security));
    }

    NMSettingIP4Config *ip4 = (NMSettingIP4Config *)nm_setting_ip4_config_new();
    g_object_set(G_OBJECT(ip4),
                 NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_AUTO,
                 NULL);
    nm_connection_add_setting(connection, NM_SETTING(ip4));

    if (nm_connection_verify(connection, &error) == FALSE) {
        LOG_ERROR << "Verification failed: " << error->message;
        InternetConnectionAvailable.blockSignals(false);
        return LastConnectResult.set(Result::InternalError);
    }

    if (nm_access_point_connection_valid(ap, connection) == FALSE) {
        LOG_ERROR << "Access point is not valid and can't be used";
        InternetConnectionAvailable.blockSignals(false);
        return LastConnectResult.set(Result::InternalError);
    }

    AddConnectionData *options = new AddConnectionData();
    options->data = m_data;
    options->Activate = true;

    nm_client_add_connection_async(m_data->Client, connection, true, NULL, Callbacks::addedNewConnection, options);

    g_main_loop_run(m_data->Loop); // Wait

    g_object_unref(connection);
    InternetConnectionAvailable.blockSignals(false);
    return LastConnectResult.value;
}

Result NetworkManager::createHotspot(std::string iface, WifiNetwork network)
{
    auto ctx = g_main_loop_get_context(m_data->Loop);
    int count = 0;
    NMDeviceWifi *device = NULL;

    NMConnection *connection = nm_simple_connection_new();

    NMSettingConnection *settingConnection = (NMSettingConnection *)nm_setting_connection_new();
    char *uuid = nm_utils_uuid_generate();
    g_object_set(G_OBJECT(settingConnection),
                 NM_SETTING_CONNECTION_UUID, uuid,
                 NM_SETTING_CONNECTION_ID, network.ssid.c_str(),
                 NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                 NM_SETTING_CONNECTION_AUTOCONNECT, FALSE,
                 NULL);
    g_free(uuid);
    nm_connection_add_setting(connection, NM_SETTING(settingConnection));

    GBytes *ssidBytes = g_bytes_new(network.ssid.data(), network.ssid.size());

    NMSettingWireless *wireless = (NMSettingWireless *)nm_setting_wireless_new();
    g_object_set(G_OBJECT(wireless),
                 NM_SETTING_WIRELESS_SSID, ssidBytes,
                 NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_AP,
                 NULL);

    g_bytes_unref(ssidBytes);

    nm_connection_add_setting(connection, NM_SETTING(wireless));

    if (network.auth == Authentication::Enterprise)
    {
        LOG_ERROR << "Enterprize networks are not supported";
        return LastConnectResult.set(Result::BadParameters);
    }

    NMSettingWirelessSecurity *security = NULL;
    if (network.auth == Authentication::WEP)
    {
        security = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
        g_object_set(G_OBJECT(security),
                     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                     NM_SETTING_WIRELESS_SECURITY_WEP_TX_KEYIDX, 0,
                     NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                     NM_SETTING_WIRELESS_SECURITY_WEP_KEY0, network.password.c_str(),
                     NULL);
        if (network.password.size() == 32)
        {
            g_object_set(G_OBJECT(security),
                         NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                         NULL);
        }
    }
    else if (network.auth == Authentication::WPA || network.auth == Authentication::WPA2)
    {
        security = (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
        g_object_set(G_OBJECT(security),
                     NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "wpa-psk",
                     NM_SETTING_WIRELESS_SECURITY_PSK, network.password.c_str(),
                     NULL);
    }

    if (security != NULL)
        nm_connection_add_setting(connection, NM_SETTING(security));

    /* Build up the 'ipv4' Setting */
    NMSettingIP4Config *ip4 = (NMSettingIP4Config *)nm_setting_ip4_config_new();
    g_object_set(G_OBJECT(ip4),
                 NM_SETTING_IP_CONFIG_METHOD, NM_SETTING_IP4_CONFIG_METHOD_SHARED,
                 NULL);
    nm_connection_add_setting(connection, NM_SETTING(ip4));

    GError *error = NULL;
    if (nm_connection_verify(connection, &error) == FALSE)
    {
        LOG_ERROR << "Verification failed: " << error->message;
        return LastConnectResult.set(Result::BadParameters);
    }
    else
    {
        AddConnectionData *options = new AddConnectionData();
        options->data = m_data;
        options->Activate = false;
        nm_client_add_connection_async(m_data->Client, connection, true, NULL, Callbacks::addedNewConnection, options);
    }

    g_main_loop_run(m_data->Loop); // Wait
    g_object_unref(connection);

    return m_data->LastConnectResult.value;
}

NetworkManager &NetworkManager::i()
{
    static NetworkManager instance;
    return instance;
}
