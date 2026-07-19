using System;
using System.Collections.Generic;
using System.IO;

namespace ESXiminator.Hub
{
    internal static class Prefs
    {
        static Dictionary<string, string> Load()
        {
            var d = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            try
            {
                if (!File.Exists(AppPaths.PrefsFile)) return d;
                foreach (var line in File.ReadAllLines(AppPaths.PrefsFile))
                {
                    var i = line.IndexOf('=');
                    if (i <= 0) continue;
                    d[line.Substring(0, i).Trim()] = line.Substring(i + 1).Trim();
                }
            }
            catch { }
            return d;
        }

        static void Save(Dictionary<string, string> d)
        {
            Directory.CreateDirectory(AppPaths.PrefsDir);
            using (var w = new StreamWriter(AppPaths.PrefsFile))
                foreach (var kv in d)
                    w.WriteLine(kv.Key + "=" + kv.Value);
        }

        public static bool AutoCheckUpdates
        {
            get
            {
                var d = Load();
                return !d.TryGetValue("AutoCheckUpdates", out var v) || v != "0";
            }
            set
            {
                var d = Load();
                d["AutoCheckUpdates"] = value ? "1" : "0";
                Save(d);
            }
        }

        public static string LastVstInstallDir
        {
            get
            {
                var d = Load();
                return d.TryGetValue("LastVstInstallDir", out var v) ? v : "";
            }
            set
            {
                var d = Load();
                if (string.IsNullOrWhiteSpace(value)) d.Remove("LastVstInstallDir");
                else d["LastVstInstallDir"] = value;
                Save(d);
            }
        }
    }
}
