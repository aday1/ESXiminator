using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace ESXiminator.Hub
{
    internal sealed class ReleaseInfo
    {
        public string Tag;
        public string Version;
        public string FullZipUrl;
        public string Notes;
        public bool IsNewerThan(string local)
        {
            return CompareVersions(Version, local) > 0;
        }

        public static int CompareVersions(string a, string b)
        {
            var pa = Parse(a);
            var pb = Parse(b);
            for (int i = 0; i < 3; i++)
            {
                if (pa[i] != pb[i]) return pa[i].CompareTo(pb[i]);
            }
            return 0;
        }

        static int[] Parse(string v)
        {
            v = (v ?? "0").Trim().TrimStart('v', 'V');
            var parts = v.Split('.');
            var r = new int[3];
            for (int i = 0; i < 3; i++)
                if (i < parts.Length) int.TryParse(Regex.Match(parts[i], @"\d+").Value, out r[i]);
            return r;
        }
    }

    internal static class GitHubUpdate
    {
        const string ApiLatest = "https://api.github.com/repos/aday1/ESXiminator/releases/latest";
        static readonly HttpClient Http;

        static GitHubUpdate()
        {
            Http = new HttpClient();
            Http.DefaultRequestHeaders.UserAgent.ParseAdd("ESXiminator-Hub/2.0");
            Http.DefaultRequestHeaders.Accept.ParseAdd("application/vnd.github+json");
            Http.Timeout = TimeSpan.FromSeconds(45);
        }

        public static async Task<ReleaseInfo> FetchLatestAsync()
        {
            var json = await Http.GetStringAsync(ApiLatest).ConfigureAwait(false);
            var tag = Match(json, "\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
            var body = Unescape(Match(json, "\"body\"\\s*:\\s*\"((?:\\\\.|[^\"\\\\])*)\""));
            var urls = Regex.Matches(json, "\"browser_download_url\"\\s*:\\s*\"([^\"]+)\"");
            string full = null;
            foreach (Match m in urls)
            {
                var u = m.Groups[1].Value;
                if (u.IndexOf("win64", StringComparison.OrdinalIgnoreCase) >= 0)
                    full = u;
            }
            if (full == null && urls.Count > 0)
                full = urls.Cast<Match>().Select(m => m.Groups[1].Value)
                    .FirstOrDefault(u => u.EndsWith(".zip", StringComparison.OrdinalIgnoreCase));

            return new ReleaseInfo
            {
                Tag = tag,
                Version = tag.TrimStart('v', 'V'),
                FullZipUrl = full,
                Notes = body
            };
        }

        public static async Task DownloadAndApplyAsync(string zipUrl, IProgress<string> progress)
        {
            if (string.IsNullOrEmpty(zipUrl))
                throw new InvalidOperationException("No release zip URL found.");

            var tmpRoot = Path.Combine(Path.GetTempPath(), "ESXiminator-update-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(tmpRoot);
            var zipPath = Path.Combine(tmpRoot, "release.zip");
            try
            {
                progress?.Report("Downloading update...");
                using (var resp = await Http.GetAsync(zipUrl, HttpCompletionOption.ResponseHeadersRead).ConfigureAwait(false))
                {
                    resp.EnsureSuccessStatusCode();
                    using (var fs = File.Create(zipPath))
                        await resp.Content.CopyToAsync(fs).ConfigureAwait(false);
                }

                progress?.Report("Extracting...");
                var extract = Path.Combine(tmpRoot, "extract");
                ZipFile.ExtractToDirectory(zipPath, extract);

                // Zip may be flat or have a single root folder
                var src = extract;
                var kids = Directory.GetDirectories(extract);
                if (kids.Length == 1 && !File.Exists(Path.Combine(extract, "ESXiminator.exe"))
                    && !Directory.Exists(Path.Combine(extract, "ESXiminator.vst3")))
                    src = kids[0];

                progress?.Report("Installing files...");
                CopyFileIfExists(Path.Combine(src, "ESXiminator.exe"),
                    Path.Combine(AppPaths.HubDir, "ESXiminator.exe"));
                CopyFileIfExists(Path.Combine(src, "ESXiminator-debug.exe"),
                    Path.Combine(AppPaths.HubDir, "ESXiminator-debug.exe"));
                CopyFileIfExists(Path.Combine(src, "ESXiminator-Hub.exe"),
                    Path.Combine(AppPaths.HubDir, "ESXiminator-Hub.exe.new"));
                CopyFileIfExists(Path.Combine(src, "version.txt"), AppPaths.VersionFile);
                CopyFileIfExists(Path.Combine(src, "GETTING_STARTED.md"),
                    Path.Combine(AppPaths.HubDir, "GETTING_STARTED.md"));
                CopyFileIfExists(Path.Combine(src, "Install-VST.bat"),
                    Path.Combine(AppPaths.HubDir, "Install-VST.bat"));
                CopyFileIfExists(Path.Combine(src, "Install-VST-User.bat"),
                    Path.Combine(AppPaths.HubDir, "Install-VST-User.bat"));

                var vstSrc = Path.Combine(src, "ESXiminator.vst3");
                if (Directory.Exists(vstSrc))
                {
                    var vstDst = AppPaths.PackageVstBundle;
                    if (Directory.Exists(vstDst))
                        Directory.Delete(vstDst, true);
                    CopyDirectory(vstSrc, vstDst);
                }

                // Refresh preferred / already-installed VST location
                if (AppPaths.VstLooksInstalled(AppPaths.PreferredVstBundle)
                    || Directory.Exists(AppPaths.PreferredVstBundle)
                    || AppPaths.VstLooksInstalled(AppPaths.SystemVstDir)
                    || Directory.Exists(AppPaths.SystemVstDir)
                    || AppPaths.VstLooksInstalled(AppPaths.UserVstDir)
                    || Directory.Exists(AppPaths.UserVstDir))
                {
                    progress?.Report("Refreshing VST3 at: " + AppPaths.PreferredVstBundle);
                    VstInstaller.RefreshPreferredInstall();
                }

                progress?.Report("Update complete.");
            }
            finally
            {
                try { Directory.Delete(tmpRoot, true); } catch { }
            }
        }

        static void CopyFileIfExists(string src, string dst)
        {
            if (!File.Exists(src)) return;
            Directory.CreateDirectory(Path.GetDirectoryName(dst) ?? ".");
            File.Copy(src, dst, true);
        }

        static void CopyDirectory(string src, string dst)
        {
            Directory.CreateDirectory(dst);
            foreach (var file in Directory.GetFiles(src, "*", SearchOption.AllDirectories))
            {
                var rel = file.Substring(src.Length).TrimStart('\\', '/');
                var target = Path.Combine(dst, rel);
                Directory.CreateDirectory(Path.GetDirectoryName(target) ?? ".");
                File.Copy(file, target, true);
            }
        }

        static string Match(string json, string pattern)
        {
            var m = Regex.Match(json, pattern);
            return m.Success ? m.Groups[1].Value : "";
        }

        static string Unescape(string s) =>
            string.IsNullOrEmpty(s) ? "" :
            s.Replace("\\n", "\n").Replace("\\r", "").Replace("\\\"", "\"").Replace("\\\\", "\\");
    }
}
