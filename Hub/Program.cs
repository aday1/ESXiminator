using System;
using System.Windows.Forms;

namespace ESXiminator.Hub
{
    internal static class Program
    {
        [STAThread]
        static int Main(string[] args)
        {
            for (int i = 0; i < args.Length; i++)
            {
                try
                {
                    if (args[i] == "--install-vst-system")
                    {
                        VstInstaller.InstallSystem();
                        return 0;
                    }
                    if (args[i] == "--install-vst-to" && i + 1 < args.Length)
                    {
                        VstInstaller.InstallToPath(args[++i].Trim('"'));
                        return 0;
                    }
                    if (args[i] == "--uninstall-vst")
                    {
                        VstInstaller.Uninstall();
                        return 0;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "ESXiminator Hub");
                    return 1;
                }
            }

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
            return 0;
        }
    }
}
