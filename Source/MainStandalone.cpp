// ESXiminator standalone launcher (custom app so we skip JUCE's standalone client TU)
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include <iostream>

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

class ESXiminatorApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "ESXiminator"; }
    const juce::String getApplicationVersion() override { return "2.0.2"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String&) override
    {
        std::cout << "[esx] initialise\n" << std::flush;
        try
        {
            juce::PropertiesFile::Options o;
            o.applicationName = "ESXiminator";
            o.filenameSuffix  = ".settings";
            o.folderName      = "ESXiminator";
            o.osxLibrarySubFolder = "Application Support";
            props = std::make_unique<juce::PropertiesFile> (o);
            std::cout << "[esx] settings ok\n" << std::flush;

            window = std::make_unique<juce::StandaloneFilterWindow> (
                "ESXiminator", juce::Colour (0xff232022), props.get(), false);
            std::cout << "[esx] window created\n" << std::flush;
            window->setUsingNativeTitleBar (true);
            window->setVisible (true);
            std::cout << "[esx] visible\n" << std::flush;
        }
        catch (const std::exception& e)
        {
            std::cout << "[esx] EXCEPTION: " << e.what() << "\n" << std::flush;
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "ESXiminator failed to start", e.what());
        }
        catch (...)
        {
            std::cout << "[esx] UNKNOWN EXCEPTION\n" << std::flush;
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                "ESXiminator failed to start", "Unknown error during startup");
        }
    }

    void shutdown() override { window = nullptr; props = nullptr; }
    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<juce::PropertiesFile> props;
    std::unique_ptr<juce::StandaloneFilterWindow> window;
};

START_JUCE_APPLICATION (ESXiminatorApp)
