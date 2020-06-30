#include "callbacks.h"
#include "networksignals.h"
#include "utilities.h"
#include "log.h"
#include "nm_private_data.h"

using namespace IoT;

void Callbacks::scanCompleted(GObject *device, GAsyncResult *result, gpointer user_data)
{
    NMDeviceWifi *wifi = NM_DEVICE_WIFI (device);
    WifiScanData* data = (WifiScanData*)user_data;
    GError *error = NULL;
    std::unique_lock<std::mutex> lock(data->mx);

    if (!nm_device_wifi_request_scan_finish (wifi, result, &error)) {
        if (error != NULL) {
            if (data->force) {
                std::string cmd = "iwlist ";
                cmd += nm_device_get_iface(NM_DEVICE(wifi));
                cmd += " scan";
                system(cmd.c_str());
            }
            LOG_ERROR << "Error on finishing scan: " << error->message;
            g_error_free(error);
        }
    }
    data->cv.notify_all();
}

void Callbacks::connectionActivated(GObject *client, GAsyncResult *result, gpointer user_data) {
    AddConnectionData *data = (AddConnectionData *)user_data;
    GError *error = NULL;

    NMActiveConnection* active = nm_client_activate_connection_finish(NM_CLIENT(client), result, &error);

    if (error) {
        LOG_ERROR << "Error activating connection: " << error->message;
        data->data->LastConnectResult.set(Result::BadCredentials);
        g_error_free(error);
    } else {
        NMRemoteConnection* remote = nm_active_connection_get_connection(active);
        LOG_DEBUG << "Activated: " << nm_connection_get_path(NM_CONNECTION(remote));
        bool ok = true;
        data->data->LastConnectResult.set(Result::Connected);
    }
    g_main_loop_quit (data->data->Loop);
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
        data->data->LastConnectResult.set(Result::BadParameters);
        g_error_free(error);
        delete data;
        g_main_loop_quit (data->data->Loop);
        return;
    } else {
        LOG_DEBUG << "Added: " << nm_connection_get_path(NM_CONNECTION(remote));
    }

    data->data->LastConnectResult.set(Result::Added);
    if (data->Activate) {
        LOG_DEBUG << "Activating...";
        nm_client_activate_connection_async(NM_CLIENT(client), NM_CONNECTION(remote), 
                                            NULL,
                                            NULL, NULL,
                                            Callbacks::connectionActivated,
                                            data);
    } else {
        g_main_loop_quit (data->data->Loop);
        delete data;
    }
}