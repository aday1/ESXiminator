using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ESXiminator.Hub
{
    internal sealed class MainForm : Form
    {
        readonly Panel header = new Panel();
        readonly Label status = new Label();
        readonly Label versionLbl = new Label();
        readonly ProgressBar progress = new ProgressBar();
        readonly CheckBox autoCheck = new CheckBox();
        readonly Button btnLaunch = new Button();
        readonly Button btnInstallVst = new Button();
        readonly Button btnUninstall = new Button();
        readonly Button btnCheck = new Button();
        readonly Button btnUpdate = new Button();
        readonly Button btnDocs = new Button();
        readonly Button btnGithub = new Button();
        bool busy;

        public MainForm()
        {
            Text = "ESXiminator Hub";
            Width = 560;
            Height = 540;
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = Theme.Panel;
            Font = Theme.UiFont;
            ForeColor = Theme.Text;
            DoubleBuffered = true;

            header.Dock = DockStyle.Top;
            header.Height = 72;
            header.Paint += (s, e) => Theme.PaintMetalHeader(e.Graphics, header.ClientRectangle,
                "ESXiminator", "SETUP  /  LAUNCH  /  UPDATE");
            Controls.Add(header);

            versionLbl.AutoSize = false;
            versionLbl.SetBounds(24, 90, 500, 22);
            versionLbl.ForeColor = Theme.Amber;
            versionLbl.Font = Theme.BoldFont;
            Controls.Add(versionLbl);

            status.AutoSize = false;
            status.SetBounds(24, 114, 500, 72);
            status.ForeColor = Theme.Muted;
            Controls.Add(status);

            progress.SetBounds(24, 190, 500, 14);
            progress.Style = ProgressBarStyle.Marquee;
            progress.MarqueeAnimationSpeed = 0;
            progress.Visible = false;
            Controls.Add(progress);

            btnLaunch.Text = "Launch ESXiminator";
            btnLaunch.SetBounds(24, 214, 500, 44);
            Theme.StylePrimaryButton(btnLaunch);
            btnLaunch.Click += async (s, e) => await Safe(LaunchAsync);
            Controls.Add(btnLaunch);

            btnInstallVst.Text = "Install VST3...";
            btnInstallVst.SetBounds(24, 272, 245, 36);
            Theme.StyleSecondaryButton(btnInstallVst);
            btnInstallVst.Click += async (s, e) => await Safe(InstallVstAsync);
            Controls.Add(btnInstallVst);

            btnUninstall.Text = "Remove VST3";
            btnUninstall.SetBounds(279, 272, 245, 36);
            Theme.StyleSecondaryButton(btnUninstall);
            btnUninstall.Click += async (s, e) => await Safe(UninstallAsync);
            Controls.Add(btnUninstall);

            btnCheck.Text = "Check for updates";
            btnCheck.SetBounds(24, 318, 245, 36);
            Theme.StyleSecondaryButton(btnCheck);
            btnCheck.Click += async (s, e) => await Safe(() => CheckUpdatesAsync(true));
            Controls.Add(btnCheck);

            btnUpdate.Text = "Download & install update";
            btnUpdate.SetBounds(24, 364, 500, 40);
            Theme.StylePrimaryButton(btnUpdate);
            btnUpdate.Enabled = false;
            btnUpdate.Click += async (s, e) => await Safe(ApplyUpdateAsync);
            Controls.Add(btnUpdate);

            autoCheck.Text = "Automatically check GitHub for updates when Hub opens";
            autoCheck.SetBounds(24, 416, 500, 24);
            autoCheck.ForeColor = Theme.Text;
            autoCheck.Checked = Prefs.AutoCheckUpdates;
            autoCheck.CheckedChanged += (s, e) => Prefs.AutoCheckUpdates = autoCheck.Checked;
            Controls.Add(autoCheck);

            btnDocs.Text = "Getting started";
            btnDocs.SetBounds(24, 452, 245, 32);
            Theme.StyleSecondaryButton(btnDocs);
            btnDocs.Click += (s, e) => OpenDocs();
            Controls.Add(btnDocs);

            btnGithub.Text = "GitHub / showcase";
            btnGithub.SetBounds(279, 452, 245, 32);
            Theme.StyleSecondaryButton(btnGithub);
            btnGithub.Click += (s, e) =>
            {
                Process.Start(new ProcessStartInfo("https://aday1.github.io/ESXiminator/") { UseShellExecute = true });
            };
            Controls.Add(btnGithub);

            Shown += async (s, e) =>
            {
                RefreshStatus();
                if (Prefs.AutoCheckUpdates)
                    await Safe(() => CheckUpdatesAsync(false));
            };
        }

        ReleaseInfo pending;

        void RefreshStatus()
        {
            versionLbl.Text = "Local version  v" + AppPaths.LocalVersion;
            var bits = new System.Text.StringBuilder();
            var exe = AppPaths.ResolveStandaloneExe();
            bits.AppendLine(File.Exists(exe)
                ? "Standalone ready: " + exe
                : "Standalone missing — use the MSI, or unzip the full win64 package next to this Hub.");
            bits.Append(VstInstaller.StatusText());
            status.Text = bits.ToString();
        }

        void SetBusy(bool on, string msg = null)
        {
            busy = on;
            progress.Visible = on;
            progress.MarqueeAnimationSpeed = on ? 30 : 0;
            foreach (Control c in new Control[] {
                btnLaunch, btnInstallVst, btnUninstall, btnCheck, btnUpdate, autoCheck })
                c.Enabled = !on && (c != btnUpdate || pending != null);
            if (!string.IsNullOrEmpty(msg))
                status.Text = msg;
        }

        async Task Safe(Func<Task> work)
        {
            if (busy) return;
            try
            {
                await work();
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, ex.Message, "ESXiminator Hub", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                status.ForeColor = Theme.Led;
                status.Text = ex.Message;
            }
            finally
            {
                SetBusy(false);
                RefreshStatus();
                btnUpdate.Enabled = pending != null && pending.IsNewerThan(AppPaths.LocalVersion);
            }
        }

        Task LaunchAsync()
        {
            var exe = AppPaths.ResolveStandaloneExe();
            if (!File.Exists(exe))
                throw new InvalidOperationException(
                    "ESXiminator.exe not found.\n\nUse the MSI installer, or unzip the full win64 package "
                    + "so ESXiminator.exe sits next to ESXiminator-Hub.exe.");
            Process.Start(new ProcessStartInfo(exe) { UseShellExecute = true });
            return Task.CompletedTask;
        }

        Task InstallVstAsync()
        {
            var dst = VstInstaller.InstallWithFolderPicker(this);
            if (dst == null) return Task.CompletedTask;
            MessageBox.Show(this,
                "VST installed here:\n\n" + dst + "\n\nRescan plugins in your DAW.",
                "VST installed", MessageBoxButtons.OK, MessageBoxIcon.Information);
            status.ForeColor = Theme.Amber;
            status.Text = "VST ends up at: " + dst;
            return Task.CompletedTask;
        }

        Task UninstallAsync()
        {
            if (MessageBox.Show(this, "Remove installed ESXiminator VST3?", "Confirm",
                    MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
                return Task.CompletedTask;
            SetBusy(true, "Removing VST3...");
            VstInstaller.Uninstall();
            status.ForeColor = Theme.Amber;
            status.Text = "VST3 removed.";
            return Task.CompletedTask;
        }

        async Task CheckUpdatesAsync(bool interactive)
        {
            SetBusy(true, "Checking GitHub for updates...");
            status.ForeColor = Theme.Muted;
            var info = await GitHubUpdate.FetchLatestAsync();
            pending = info;
            if (info == null || string.IsNullOrEmpty(info.Version))
                throw new InvalidOperationException("Could not read GitHub release info.");

            if (info.IsNewerThan(AppPaths.LocalVersion))
            {
                status.ForeColor = Theme.Led;
                status.Text = "Update available: v" + info.Version + "  (you have v" + AppPaths.LocalVersion + ")";
                btnUpdate.Enabled = true;
                if (interactive)
                {
                    var r = MessageBox.Show(this,
                        "ESXiminator v" + info.Version + " is available.\n\nDownload and install now?",
                        "Update available", MessageBoxButtons.YesNo, MessageBoxIcon.Information);
                    if (r == DialogResult.Yes)
                        await ApplyUpdateAsync();
                }
            }
            else
            {
                status.ForeColor = Theme.Amber;
                status.Text = "You're up to date (v" + AppPaths.LocalVersion + "). Latest on GitHub: v" + info.Version;
                btnUpdate.Enabled = false;
                if (interactive)
                    MessageBox.Show(this, "You're on the latest release.", "ESXiminator Hub");
            }
        }

        async Task ApplyUpdateAsync()
        {
            if (pending == null || string.IsNullOrEmpty(pending.FullZipUrl))
            {
                await CheckUpdatesAsync(false);
                if (pending == null || string.IsNullOrEmpty(pending.FullZipUrl))
                    throw new InvalidOperationException("No update package URL.");
            }
            var prog = new Progress<string>(m =>
            {
                status.ForeColor = Theme.Muted;
                status.Text = m;
            });
            SetBusy(true, "Updating...");
            await GitHubUpdate.DownloadAndApplyAsync(pending.FullZipUrl, prog);
            pending = null;
            btnUpdate.Enabled = false;
            status.ForeColor = Theme.Amber;
            status.Text = "Updated to latest release. Launch or rescan your DAW.";
            MessageBox.Show(this, "Update installed.\nIf the Hub itself was updated, restart it to load the new Hub binary.",
                "ESXiminator Hub");
        }

        void OpenDocs()
        {
            var local = Path.Combine(AppPaths.HubDir, "GETTING_STARTED.md");
            if (File.Exists(local))
                Process.Start(new ProcessStartInfo(local) { UseShellExecute = true });
            else
                Process.Start(new ProcessStartInfo("https://github.com/aday1/ESXiminator/blob/main/GETTING_STARTED.md")
                { UseShellExecute = true });
        }
    }
}
