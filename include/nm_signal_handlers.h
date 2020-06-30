#ifndef NM_CALLBACKS_H
#define NM_CALLBACKS_H

#include "networkmanager.h"

namespace IoT {

static gboolean checkConnectivity(gpointer user_data)
{
    std::cout.flush();
    std::cerr.flush();
    Data* data = (Data*)(user_data);

    NMState state = nm_client_get_state(data->Client);
    switch(state)
    {
        case NM_STATE_CONNECTED_GLOBAL: {
            data->Connectivity = NetworkState::Connected;
            break;
        }
        case NM_STATE_CONNECTED_SITE: {
            data->Connectivity = NetworkState::Limited;
            break;
        }
        case NM_STATE_DISCONNECTING:
        case NM_STATE_DISCONNECTED:
        case NM_STATE_ASLEEP:
        case NM_STATE_CONNECTED_LOCAL: {
            data->Connectivity = NetworkState::Disconnected;
            break;
        }
        default: {
            data->Connectivity = NetworkState::UnknownYet;
            break;
        }
    }

    return TRUE;
}

static void onConnectionAddedReceived(NMClient*client, NMRemoteConnection *connection, gpointer user_data)
{
    Data* data = (Data*)(user_data);
    if (data == NULL) {
        LOG_ERROR << "Internal error in glib slot";
    }

    bool ok = true;
    Connection c = connectionFromNM(NM_CONNECTION(connection), ok);
    if (!ok) {
        return;
    }

    data->ConnectionAdded.emit(c);
}

static void onDeviceStateChanged(NMDevice *device, guint new_state, guint old_state, guint reason, gpointer user_data)
{
    Data* data = (Data*)(user_data);
    if (data == NULL) {
        LOG_ERROR << "Internal error in glib slot";
    }

    if (!NM_IS_DEVICE_WIFI(device)) {
        return;
    }

    bool ok = false;
    WifiNetwork net = getCurrentNetwork(NM_DEVICE_WIFI(device), ok);
    if (ok) {
        data->DeviceWiFiNetworkChanged.emit(std::string(nm_device_get_iface(device)), net);
    }

    NMDeviceState state = (NMDeviceState)new_state;

    NMActiveConnection* connection = nm_device_get_active_connection(device);

    Connection c;
    if (connection) {
        NMRemoteConnection* remote = nm_active_connection_get_connection(connection);
        c = connectionFromNM(NM_CONNECTION(remote), ok);
    }

    ConnectionStatus was = deviceStateToConnectionStatus((NMDeviceState) old_state);
    ConnectionStatus now = deviceStateToConnectionStatus((NMDeviceState) new_state);

    if (was != now && ok)
        data->ConnectionChanged.emit(c, now);
}

}
#endif // NM_CALBACKS_H