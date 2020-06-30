#ifndef NM_CALLBACKS_H
#define NM_CALLBACKS_H

#include <NetworkManager.h>
#include <mutex>
#include <condition_variable>

namespace IoT {

    struct CheckStateData
    {
        struct Data* data;
        GMainLoop* Loop;
    };
    struct SignalHandler
    {
        static gboolean checkConnectivity(gpointer user_data);

        static void onConnectionAddedReceived(NMClient*client, NMRemoteConnection *connection, gpointer user_data);
    };

    struct AddConnectionData
    {
        struct Data* data;
        bool Activate;
    };

    struct WifiScanData
    {
        std::mutex mx;
        std::condition_variable cv;
        bool force = false;
    };

    struct Callbacks
    {
        static void scanCompleted(GObject *device, GAsyncResult *result, gpointer user_data);
        static void connectionActivated(GObject *client, GAsyncResult *result, gpointer user_data);
        static void addedNewConnection(GObject *client, GAsyncResult *result, gpointer user_data);
    };
}
#endif // NM_CALBACKS_H