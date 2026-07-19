using System;
using System.IO;
using System.Reflection;
using Microsoft.Win32;

namespace ESXiminator.Hub
{
    internal static class AppPaths
    {
        public static string HubDir =>
            Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? ".";

        public static string VersionFile => Path.Combine(HubDir, "version.txt");

        /// <summary>Default preferred VST3 parent: Common Files\VST3</summary>
        public static string DefaultVstParentDir =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3");

        public static string SystemVstDir =>
            Path.Combine(DefaultVstParentDir, "ESXiminator.vst3");

        public static string UserVstDir =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "Programs", "Common", "VST3", "ESXiminator.vst3");

        public static string ProgramFilesAppDir =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "ESXiminator");

        public static string PrefsDir =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "ESXiminator");

        public static string PrefsFile => Path.Combine(PrefsDir, "hub-prefs.ini");

        /// <summary>
        /// Parent folder for ESXiminator.vst3: Hub pref, then MSI registry, then Common Files\VST3.
        /// </summary>
        public static string PreferredVstParentDir
        {
            get
            {
                var fromPref = Prefs.LastVstInstallDir;
                if (!string.IsNullOrWhiteSpace(fromPref) && Directory.Exists(fromPref))
                    return fromPref.TrimEnd('\\', '/');

                var fromReg = ReadRegistryVstInstallDir();
                if (!string.IsNullOrWhiteSpace(fromReg) && Directory.Exists(fromReg))
                    return fromReg.TrimEnd('\\', '/');

                return DefaultVstParentDir;
            }
        }

        public static string PreferredVstBundle =>
            Path.Combine(PreferredVstParentDir, "ESXiminator.vst3");

        public static void RememberPreferredVstParent(string parentOrBundle)
        {
            if (string.IsNullOrWhiteSpace(parentOrBundle)) return;
            var path = parentOrBundle.TrimEnd('\\', '/');
            if (path.EndsWith("ESXiminator.vst3", StringComparison.OrdinalIgnoreCase))
                path = Path.GetDirectoryName(path) ?? path;
            Prefs.LastVstInstallDir = path;
            try
            {
                using (var key = Registry.CurrentUser.CreateSubKey(@"Software\Aday\ESXiminator"))
                    key?.SetValue("VstInstallDir", path);
            }
            catch { }
        }

        static string ReadRegistryVstInstallDir()
        {
            try
            {
                foreach (var hive in new[] { Registry.CurrentUser, Registry.LocalMachine })
                {
                    using (var key = hive.OpenSubKey(@"Software\Aday\ESXiminator"))
                    {
                        var v = key?.GetValue("VstInstallDir") as string;
                        if (!string.IsNullOrWhiteSpace(v))
                            return v.Trim();
                    }
                }
            }
            catch { }
            return "";
        }

        public static string LocalVersion
        {
            get
            {
                try
                {
                    if (File.Exists(VersionFile))
                        return File.ReadAllText(VersionFile).Trim().TrimStart('v', 'V');
                }
                catch { }
                return Assembly.GetExecutingAssembly().GetName().Version?.ToString(3) ?? "2.0.1";
            }
        }

        public static bool VstLooksInstalled(string dir) =>
            !string.IsNullOrEmpty(dir)
            && File.Exists(Path.Combine(dir, "Contents", "x86_64-win", "ESXiminator.vst3"));

        /// <summary>Standalone next to Hub, or from Program Files MSI install.</summary>
        public static string ResolveStandaloneExe()
        {
            var candidates = new[]
            {
                Path.Combine(HubDir, "ESXiminator.exe"),
                Path.Combine(ProgramFilesAppDir, "ESXiminator.exe"),
            };
            foreach (var c in candidates)
                if (File.Exists(c)) return c;
            return Path.Combine(HubDir, "ESXiminator.exe");
        }

        /// <summary>
        /// Bundle used as the copy source for Install VST3:
        /// 1) next to Hub (zip / MSI payload)
        /// 2) already-installed system / per-user VST (repair)
        /// </summary>
        public static string ResolveVstSourceBundle()
        {
            var candidates = new[]
            {
                Path.Combine(HubDir, "ESXiminator.vst3"),
                Path.Combine(ProgramFilesAppDir, "ESXiminator.vst3"),
                PreferredVstBundle,
                SystemVstDir,
                UserVstDir,
            };
            foreach (var c in candidates)
                if (VstLooksInstalled(c)) return c;
            return Path.Combine(HubDir, "ESXiminator.vst3");
        }

        /// <summary>Where Hub writes plugin files when updating the portable package.</summary>
        public static string PackageVstBundle => Path.Combine(HubDir, "ESXiminator.vst3");
    }
}
