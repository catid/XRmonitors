using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace XRmonitorsInstaller
{
    public partial class CompletePanel : UserControl
    {
        public CompletePanel()
        {
            InitializeComponent();
        }

        private void ButtonExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void ButtonLaunch_Click(object sender, EventArgs e)
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsUI.exe");

            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = exe_path;
            Process.Start(psi);

            Application.Exit();
        }

        private void LinkLabel2_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkLabel2.Text;
            Process.Start(psi);
        }

        private void LinkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkLabel1.Text;
            Process.Start(psi);
        }
    }
}
