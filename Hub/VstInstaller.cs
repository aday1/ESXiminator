using System;
using System.Diagnostics;
using System.IO;
using System.Security.Principal;
using System.Windows.Forms;

namespace ESXiminator.Hub
{
    internal static class VstInstaller
    {
        public static bool IsAdmin()
        {
            using (var id = WindowsIdentity.GetCurrent())
            {
                var p = new WindowsPrincipal(id);
                return p.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }

        public static string RequireSource()
        {
            var src = AppPaths.ResolveVstSourceBundle();
            if (!AppPaths.VstLooksInstalled(src))
                throw new InvalidOperationException(
                    "No ESXiminator.vst3 found.\n\n"
                    + "Install with the MSI, or unzip the full win64 / VST3-Setup zip so "
                    + "ESXiminator.vst3 sits next to ESXiminator-Hub.exe.");
            return src;
        }

        /// <summary>
        /// Ask for a VST3 parent folder and copy ESXiminator.vst3 into it.
        /// Returns the installed bundle path, or null if cancelled.
        /// </summary>
        public static string InstallWithFolderPicker(IWin32Window owner)
        {
            var src = RequireSource();
            var suggested = AppPaths.PreferredVstParentDir;

            using (var dlg = new FolderBrowserDialog())
            {
                dlg.Description =
                    "Choose the VST3 folder your DAW scans.\r\n"
                    + "The plugin will end up as:\r\n"
                    + suggested.TrimEnd('\\') + "\\ESXiminator.vst3\r\n"
                    + "(or that folder name under whatever path you pick).";
                dlg.ShowNewFolderButton = true;
                if (!string.IsNullOrEmpty(suggested) && Directory.Exists(suggested))
                    dlg.SelectedPath = suggested;

                if (dlg.ShowDialog(owner) != DialogResult.OK)
                    return null;

                var parent = dlg.SelectedPath.TrimEnd('\\', '/');
                // If they already picked the bundle folder, install into it directly
                var dst = AppPaths.VstLooksInstalled(parent) || parent.EndsWith("ESXiminator.vst3", StringComparison.OrdinalIgnoreCase)
                    ? parent
                    : Path.Combine(parent, "ESXiminator.vst3");

                AppPaths.RememberPreferredVstParent(dst);

                // Common Files\VST3 usually needs elevation
                var needsAdmin = parent.StartsWith(
                    Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles),
                    StringComparison.OrdinalIgnoreCase)
                    || parent.StartsWith(
                        Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                        StringComparison.OrdinalIgnoreCase);

                if (needsAdmin && !IsAdmin())
                {
                    RelaunchElevated("--install-vst-to \"" + dst + "\"");
                    return dst;
                }

                CopyBundle(src, dst);
                return dst;
            }
        }

        public static void InstallToPath(string dstBundle)
        {
            var src = RequireSource();
            if (string.IsNullOrWhiteSpace(dstBundle))
                throw new InvalidOperationException("No VST destination path.");
            var dst = dstBundle.TrimEnd('\\', '/');
            CopyBundle(src, dst);
            AppPaths.RememberPreferredVstParent(dst);
        }

        public static void InstallSystem()
        {
            var src = RequireSource();
            if (!IsAdmin())
            {
                RelaunchElevated("--install-vst-system");
                return;
            }
            CopyBundle(src, AppPaths.SystemVstDir);
            AppPaths.RememberPreferredVstParent(AppPaths.DefaultVstParentDir);
        }

        public static void InstallUser()
        {
            var src = RequireSource();
            CopyBundle(src, AppPaths.UserVstDir);
            AppPaths.RememberPreferredVstParent(Path.GetDirectoryName(AppPaths.UserVstDir));
        }

        /// <summary>Re-copy into preferred / already-installed VST location after an update.</summary>
        public static void RefreshPreferredInstall()
        {
            var preferred = AppPaths.PreferredVstBundle;
            if (AppPaths.VstLooksInstalled(preferred) || Directory.Exists(preferred))
            {
                InstallToPath(preferred);
                return;
            }
            if (AppPaths.VstLooksInstalled(AppPaths.SystemVstDir) || Directory.Exists(AppPaths.SystemVstDir))
            {
                InstallSystem();
                return;
            }
            if (AppPaths.VstLooksInstalled(AppPaths.UserVstDir) || Directory.Exists(AppPaths.UserVstDir))
                InstallUser();
        }

        public static void Uninstall()
        {
            if (Directory.Exists(AppPaths.SystemVstDir))
            {
                if (!IsAdmin())
                {
                    RelaunchElevated("--uninstall-vst");
                    return;
                }
                Directory.Delete(AppPaths.SystemVstDir, true);
            }
            if (Directory.Exists(AppPaths.UserVstDir))
                Directory.Delete(AppPaths.UserVstDir, true);

            // Also remove preferred/custom install if recorded and looks like ours
            var preferred = AppPaths.PreferredVstBundle;
            if (AppPaths.VstLooksInstalled(preferred)
                && !string.Equals(Path.GetFullPath(preferred), Path.GetFullPath(AppPaths.SystemVstDir), StringComparison.OrdinalIgnoreCase)
                && !string.Equals(Path.GetFullPath(preferred), Path.GetFullPath(AppPaths.UserVstDir), StringComparison.OrdinalIgnoreCase))
            {
                try { Directory.Delete(preferred, true); } catch { }
            }
        }

        public static string StatusText()
        {
            foreach (var path in new[]
            {
                AppPaths.PreferredVstBundle,
                AppPaths.SystemVstDir,
                AppPaths.UserVstDir,
            })
            {
                if (AppPaths.VstLooksInstalled(path))
                    return "VST ends up at: " + path;
            }

            var target = AppPaths.PreferredVstBundle;
            if (AppPaths.VstLooksInstalled(AppPaths.ResolveVstSourceBundle()))
                return "VST not installed yet. Install places it at: " + target;
            return "VST not found. After install it will be at: " + target;
        }

        static void CopyBundle(string src, string dst)
        {
            var srcFull = Path.GetFullPath(src).TrimEnd('\\');
            var dstFull = Path.GetFullPath(dst).TrimEnd('\\');
            if (string.Equals(srcFull, dstFull, StringComparison.OrdinalIgnoreCase))
                return;

            if (Directory.Exists(dst))
                Directory.Delete(dst, true);
            CopyDirectory(src, dst);
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

        static void RelaunchElevated(string args)
        {
            var psi = new ProcessStartInfo
            {
                FileName = Application.ExecutablePath,
                Arguments = args,
                UseShellExecute = true,
                Verb = "runas"
            };
            try
            {
                Process.Start(psi)?.WaitForExit(120000);
            }
            catch (System.ComponentModel.Win32Exception)
            {
                throw new InvalidOperationException("Administrator approval was cancelled.");
            }
        }
    }
}
