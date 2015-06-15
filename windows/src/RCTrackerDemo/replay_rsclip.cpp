#include "rc_intel_interface.h"
#include "TrackerManager.h"
#include "RCFactory.h"

using namespace RealityCap;

void logger(void * handle, const char * buffer_utf8, size_t length)
{
    fprintf((FILE *)handle, "%s\n", buffer_utf8);
}

int wmain(int c, wchar_t **v)
{
    if(c < 2)
    {
        fprintf(stderr, "specify file name\n");
        return 0;
    }

    RCFactory factory;
    auto trackMan = factory.CreateTrackerManager();
    trackMan->SetLog(logger, stdout);

    if (!trackMan->StartReplay(v[1], false))
    {
        fprintf(stderr, "Failed to start replay.\n");
        return 0;
    }

    while (trackMan->isVideoStreaming()) {}

    trackMan->Stop();

    fprintf(stderr, "Exiting\n");

    return 0;

}
