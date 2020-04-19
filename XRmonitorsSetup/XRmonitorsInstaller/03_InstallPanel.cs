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
using Microsoft.Win32;

namespace XRmonitorsInstaller
{
    public partial class InstallPanel : UserControl
    {
        public InstallPanel()
        {
            InitializeComponent();

            string program_files = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
            string default_path = Path.Combine(program_files, "XRmonitors");

            textInstallPath.Text = default_path;
        }

        private void ButtonBack_Click(object sender, EventArgs e)
        {
            Program.setup_form.license_panel.Show();
            this.Hide();
        }

        private void ButtonInstall_Click(object sender, EventArgs e)
        {
            bool createdNew, success = false;
            var mutex = new System.Threading.Mutex(true, "XRmonitorsUIUnique", out createdNew);
            if (mutex != null)
            {
                mutex.Dispose();
                mutex = null;
                success = true;
            }

            if (!success || !createdNew)
            {
                MessageBox.Show("An instance of XRmonitorsUI.exe is still running.  \n\nClose the software before reinstalling it.", "XRmonitors - Still Running");
                return;
            }

            try
            {
                string dir = textInstallPath.Text;
                if (dir.Length == 0)
                {
                    return;
                }
                Directory.CreateDirectory(dir);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to create directory.  Error: " + ex.ToString(), "XRmonitors - Directory Creation Failed", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                return;
            }

            Program.InstallPath = textInstallPath.Text;

            // Enable auto-start

            try
            {
                if (checkAutoStart.Checked)
                {
                    Console.WriteLine("Adding startup regkey");
                    string path = Path.Combine(Program.InstallPath, "XRmonitorsUI.exe");
                    Console.WriteLine("Found exe path: " + path);
                    Program.AddStartup("XRmonitors", path);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to set up AutoStart: ", ex.ToString());
                checkAutoStart.Enabled = false;
            }

            Program.setup_form.progress_panel.Show();
            this.Hide();

            Program.setup_form.progress_panel.Install();
        }

        private void ButtonBrowse_Click(object sender, EventArgs e)
        {
            DialogResult result = folderBrowserDialog1.ShowDialog();

            if (result == DialogResult.OK)
            {
                textInstallPath.Text = folderBrowserDialog1.SelectedPath;
            }
        }

        private void CheckAutoStart_CheckedChanged(object sender, EventArgs e)
        {
            // Do nothing until install starts
        }
    }
}
