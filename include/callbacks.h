#ifndef NM_CALLBACKS_H
#define NM_CALLBACKS_H

#include <NetworkManager.h>
#include <mutex>

namespace IoT {
    struct SignalHandler
    {
        static gboolean checkConnectivity(gpointer user_data);

        static void onConnectionAddedReceived(NMClient*client, NMRemoteConnection *connection, gpointer user_data);

        static void onDeviceStateChanged(NMDevice *device, guint new_state, guint old_state, guint reason, gpointer user_data);
    };

    struct AddConnectionData
    {
        struct Data* data;
        bool Activate;
    };

    struct WifiScanData
    {
        struct Data* data;
        std::mutex lock;
    };

    struct Callbacks
    {
        static void scanCompleted(GObject *device, GAsyncResult *result, gpointer user_data);
        static void connectionActivated(GObject *client, GAsyncResult *result, gpointer user_data);
        static void addedNewConnection(GObject *client, GAsyncResult *result, gpointer user_data);
    };
}
#endif // NM_CALBACKS_H