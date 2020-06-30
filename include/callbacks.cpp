#include "callbacks.h"
#include "networksignals.h"
#include "utilities.h"
#include "log.h"
#include "nm_private_data.h"

using namespace IoT;

gboolean SignalHandler::checkConnectivity(gpointer user_data)
{
    LOG_DEBUG << "Checking conenctivity...";
    Data* data = (Data*)(user_data);

    NMConnectivityState state = nm_client_check_connectivity(data->Client, NULL, NULL);
    switch(state)
    {
        case NM_CONNECTIVITY_FULL: {
            data->Connectivity = NetworkState::Connected;
            break;
        }
        case NM_CONNECTIVITY_LIMITED: {
            data->Connectivity = NetworkState::Limited;
            break;
        }
        case NM_CONNECTIVITY_NONE: {
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

void SignalHandler::onConnectionAddedReceived(NMClient*client, NMRemoteConnection *connection, gpointer user_data)
{
    Data* data = (Data*)(user_data);
    if (data == NULL) {
        LOG_ERROR << "Internal error in glib slot";
    }

    bool ok = true;
    Connection c = Utility::connectionFromNM(NM_CONNECTION(connection), ok);
    if (!ok) {
        return;
    }

    data->ConnectionAdded.emit(c);
}

void SignalHandler::onDeviceStateChanged(NMDevice *device, guint new_state, guint old_state, guint reason, gpointer user_data)
{
    Data* data = (Data*)(user_data);
    if (data == NULL) {
        LOG_ERROR << "Internal error in glib slot";
    }

    if (!NM_IS_DEVICE_WIFI(device)) {
        return;
    }

    bool ok = false;
    WifiNetwork net = Utility::getCurrentNetwork(NM_DEVICE_WIFI(device), ok);
    if (ok) {
        data->DeviceWiFiNetworkChanged.emit(std::string(nm_device_get_iface(device)), net);
    }

    NMDeviceState state = (NMDeviceState)new_state;

    NMActiveConnection* connection = nm_device_get_active_connection(device);

    Connection c;
    if (connection) {
        NMRemoteConnection* remote = nm_active_connection_get_connection(connection);
        c = Utility::connectionFromNM(NM_CONNECTION(remote), ok);
    }

    ConnectionStatus was = Utility::deviceStateToConnectionStatus((NMDeviceState) old_state);
    ConnectionStatus now = Utility::deviceStateToConnectionStatus((NMDeviceState) new_state);

    if (was != now && ok)
        data->ConnectionChanged.emit(c, now);
}

void Callbacks::connectionActivated(GObject *client, GAsyncResult *result, gpointer user_data) {
    AddConnectionData *data = (AddConnectionData *)user_data;
    GError *error = NULL;

    /* NM responded to our request; either handle the resulting error or
	 * print out the object path of the connection we just added.
	 */
    NMActiveConnection* active = nm_client_activate_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
        LOG_ERROR << "Error activating connection: " << error->message;
        g_error_free(error);
    } else {
        NMRemoteConnection* remote = nm_active_connection_get_connection(active);
        LOG_DEBUG << "Activated: " << nm_connection_get_path(NM_CONNECTION(remote));
        bool ok = true;
        data->data->ConnectionActivated.emit(Utility::connectionFromNM(NM_CONNECTION(remote), ok));
    }
    delete data;
}

void Callbacks::addedNewConnection(GObject *client, GAsyncResult *result, gpointer user_data)
{
    AddConnectionData *data = (AddConnectionData *)user_data;
    GError *error = NULL;

    NMRemoteConnection* remote = nm_client_add_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
        LOG_ERROR << "Error adding connection:" << error->message;
        g_error_free(error);
    } else {
        LOG_DEBUG << "Added: " << nm_connection_get_path(NM_CONNECTION(remote));
    }

    if (error) {
        g_error_free(error);
        delete data;
        return;
    }

    bool ok;
    data->data->ConnectionAdded.emit(Utility::connectionFromNM(NM_CONNECTION(remote), ok));

    if (data->Activate) {
        LOG_DEBUG << "Activating...";
        nm_client_activate_connection_async(NM_CLIENT(client), NM_CONNECTION(remote), 
                                            NULL,
                                            NULL, NULL,
                                            Callbacks::connectionActivated,
                                            data);
    } else {
        delete data;
    }
}