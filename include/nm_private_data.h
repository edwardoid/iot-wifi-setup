#ifndef IOT_NM_PRIVATE_DATA
#define IOT_NM_PRIVATE_DATA

#include "networksignals.h"
namespace IoT
{
    struct Data: public NetworkSignals
    {
        NMClient* Client;
        GMainLoop* Loop;
    };
}

#endif // IOT_NM_PRIVATE_DATA