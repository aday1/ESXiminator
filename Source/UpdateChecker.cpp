#include "UpdateChecker.h"
#include "Version.h"

#if JUCE_WINDOWS
 #include <windows.h>
 #include <wininet.h>
 #pragma comment(lib, "wininet.lib")
#endif

namespace esx
{

juce::File UpdatePrefs::prefsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("ESXiminator")
               .getChildFile ("prefs.ini");
}

bool UpdatePrefs::getCheckEnabled()
{
    auto f = prefsFile();
    if (! f.existsAsFile()) return false; // opt-in default OFF
    return f.loadFileAsString().containsIgnoreCase ("CheckForUpdates=1");
}

void UpdatePrefs::setCheckEnabled (bool on)
{
    auto f = prefsFile();
    f.getParentDirectory().createDirectory();
    juce::StringArray lines;
    if (f.existsAsFile())
        lines.addLines (f.loadFileAsString());
    bool found = false;
    for (auto& l : lines)
        if (l.startsWithIgnoreCase ("CheckForUpdates="))
        {
            l = on ? "CheckForUpdates=1" : "CheckForUpdates=0";
            found = true;
        }
    if (! found)
        lines.add (on ? "CheckForUpdates=1" : "CheckForUpdates=0");
    f.replaceWithText (lines.joinIntoString ("\n"));
}

juce::String UpdateChecker::localVersion()
{
    return ESXIMINATOR_VERSION_STRING;
}

#if JUCE_WINDOWS
static juce::String httpGet (const juce::String& url)
{
    juce::String result;
    HINTERNET sess = InternetOpenW (L"ESXiminator/1.2", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (sess == nullptr) return {};

    HINTERNET req = InternetOpenUrlW (sess, url.toWideCharPointer(),
                                      L"Accept: application/vnd.github+json\r\nUser-Agent: ESXiminator/1.2\r\n",
                                      (DWORD) -1,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE,
                                      0);
    if (req == nullptr)
    {
        InternetCloseHandle (sess);
        return {};
    }

    char buf[4096];
    DWORD read = 0;
    while (InternetReadFile (req, buf, sizeof (buf), &read) && read > 0)
        result += juce::String::fromUTF8 (buf, (int) read);

    InternetCloseHandle (req);
    InternetCloseHandle (sess);
    return result;
}
#else
static juce::String httpGet (const juce::String&) { return {}; }
#endif

static int verPart (const juce::String& v, int idx)
{
    auto t = v.trim().trimCharactersAtStart ("vV");
    auto parts = juce::StringArray::fromTokens (t, ".", "");
    if (idx >= parts.size()) return 0;
    return parts[idx].retainCharacters ("0123456789").getIntValue();
}

static int compareVer (const juce::String& a, const juce::String& b)
{
    for (int i = 0; i < 3; ++i)
    {
        const int d = verPart (a, i) - verPart (b, i);
        if (d != 0) return d;
    }
    return 0;
}

static juce::String jsonStringField (const juce::String& json, const juce::String& key)
{
    auto marker = "\"" + key + "\"";
    auto rest = json.fromFirstOccurrenceOf (marker, false, false);
    if (rest.isEmpty()) return {};
    rest = rest.fromFirstOccurrenceOf (":", false, false).trimStart();
    if (! rest.startsWithChar ('"')) return {};
    rest = rest.substring (1);
    juce::String out;
    for (int i = 0; i < rest.length(); ++i)
    {
        auto c = rest[i];
        if (c == '\\' && i + 1 < rest.length())
        {
            auto n = rest[++i];
            if (n == 'n') out += '\n';
            else if (n == '"') out += '"';
            else if (n == '\\') out += '\\';
            else out += n;
            continue;
        }
        if (c == '"') break;
        out += c;
    }
    return out;
}

UpdateResult UpdateChecker::checkLatest()
{
    UpdateResult r;
    r.localVersion = localVersion();
    const auto json = httpGet ("https://api.github.com/repos/aday1/ESXiminator/releases/latest");
    if (json.isEmpty())
    {
        r.message = "Could not reach GitHub (offline or blocked).";
        return r;
    }

    const auto tag = jsonStringField (json, "tag_name");
    if (tag.isEmpty())
    {
        r.message = "Unexpected GitHub response.";
        return r;
    }

    r.ok = true;
    r.remoteVersion = tag.trimCharactersAtStart ("vV");
    r.downloadUrl = "https://github.com/aday1/ESXiminator/releases/latest";

    int searchFrom = 0;
    while (searchFrom >= 0 && searchFrom < json.length())
    {
        const int key = json.indexOfIgnoreCase (searchFrom, "\"browser_download_url\"");
        if (key < 0) break;
        auto slice = json.substring (key);
        auto url = jsonStringField (slice, "browser_download_url");
        if (url.containsIgnoreCase ("win64"))
        {
            r.downloadUrl = url;
            break;
        }
        searchFrom = key + 20;
    }

    if (compareVer (r.remoteVersion, r.localVersion) > 0)
    {
        r.updateAvailable = true;
        r.message = "Update available: v" + r.remoteVersion + " (you have v" + r.localVersion + ")";
    }
    else
    {
        r.message = "Up to date (v" + r.localVersion + ")";
    }
    return r;
}

void UpdateChecker::openDownloadPage (const UpdateResult& r)
{
    const auto url = r.downloadUrl.isNotEmpty() ? r.downloadUrl
                    : juce::String ("https://github.com/aday1/ESXiminator/releases/latest");
    juce::URL (url).launchInDefaultBrowser();
}

} // namespace esx
