using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsUI
{
    public partial class OpenXrNeeded : Form
    {
        public OpenXrNeeded()
        {
            InitializeComponent();
        }

        private void SetWebText(string s)
        {
            if (webBrowser.DocumentText != s)
            {
                webBrowser.DocumentText = s;
            }
        }

        private void UpdateText()
        {
            int release_id = Program.GetReleaseId();

            labelReleaseId.Text = "Windows Release " + release_id.ToString();

            if (release_id < 1809)
            {
                // Must upgrade to October 2018 update or newer
                SetWebText(
@"
<h2>Needed: Windows Update</h2>
<p>Please follow the link below to upgrade the computer to the Windows 10 October 2018 update (Version 1809) or newer.</p>
<p>You may also use the Windows Update settings panel to upgrade your operating system: <b>""ms-settings:windowsupdate""</b>
We have found that sometimes 3rd party applications will interfere with the settings panel upgrade, so we recommend using the link below if possible.</p>
<p>When complete, you may need to reboot.  To retry click the Retry button below.</p>
");

                linkLabel.Text = "https://www.microsoft.com/en-us/software-download/windows10";
                return;
            }
            else if (release_id < 1903)
            {
                if (!Program.IsProgramInstalled("Mixed Reality OpenXR Developer Preview Compatibility Pack"))
                {
                    SetWebText(
@"
<h2>Needed: OpenXR Compatibility Pack</h2>
<p>Please follow the link below to download the OpenXR Compatibility Pack.  Run the <b>MixedRealityRuntimeCompatPack.exe</b> to continue.</p>
<p>Alternatively you can upgrade to the May 2019 release of Windows 10:</p>
<p><i>https://www.microsoft.com/en-us/software-download/windows10</i></p>
<p>This software is provided by Microsoft to enable OpenXR.  To retry click the Retry button below.</p>
");

                    linkLabel.Text = "https://aka.ms/openxr-compat";
                    return;
                }
            }

            // If OpenXR runtime is not installed:
            if (!Program.IsStoreAppInstalled("Microsoft.WindowsMixedRealityRuntimeApp") &&
                !Program.IsStoreAppInstalled("Microsoft.MixedRealityRuntimeDeveloperPreview"))
            {
                // Prompt them to install the user version
                SetWebText(
@"
<h2>Needed: Mixed Reality OpenXR Runtime</h2>
<p>Please follow the link below to install the Mixed Reality OpenXR Runtime.</p>
<p>This software is provided by Microsoft to enable OpenXR.  To retry click the Retry button below.</p>
");

                linkLabel.Text = "https://www.microsoft.com/en-us/p/openxr-for-windows-mixed-reality/9p9596djj19r";
                return;
            }

            SetWebText(
@"
<h2>No problems found</h2>
<p>It seems like all required OpenXR software is installed.  You may need to reboot before changes will take effect.</p>
<p>To retry click the Retry button below.</p>
");

            linkLabel.Text = "https://docs.microsoft.com/en-us/windows/mixed-reality/openxr";
        }

        private void ButtonOkay_Click(object sender, EventArgs e)
        {
            Program.ShowMainForm(); this.Hide();
        }

        private void ButtonExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void OpenXrNeeded_Activated(object sender, EventArgs e)
        {
            UpdateText();
        }

        private void LinkLabel_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkLabel.Text;
            Process.Start(psi);
        }

        private void OpenXrNeeded_Load(object sender, EventArgs e)
        {
            UpdateText();
        }

        private void Timer1_Tick(object sender, EventArgs e)
        {
            if (!this.Visible)
            {
                return;
            }

            UpdateText();
            System.Threading.Thread.Sleep(100);
        }
    }
}
