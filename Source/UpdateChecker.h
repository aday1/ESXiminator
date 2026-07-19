#pragma once
#include <juce_core/juce_core.h>

namespace esx
{
/** Opt-in GitHub release checker (WinINet). Prefs live in %AppData%/ESXiminator/prefs.ini */
struct UpdatePrefs
{
    static juce::File prefsFile();
    static bool getCheckEnabled();
    static void setCheckEnabled (bool on);
};

struct UpdateResult
{
    bool ok = false;
    bool updateAvailable = false;
    juce::String localVersion;
    juce::String remoteVersion;
    juce::String downloadUrl;
    juce::String message;
};

class UpdateChecker
{
public:
    static juce::String localVersion();
    static UpdateResult checkLatest();
    static void openDownloadPage (const UpdateResult& r);
};
}
