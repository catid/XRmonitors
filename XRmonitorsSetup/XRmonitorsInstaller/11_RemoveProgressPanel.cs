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
using System.Threading;
using System.Diagnostics;
using Microsoft.Win32;
using System.Reflection;

namespace XRmonitorsInstaller
{
    public partial class RemoveProgressPanel : UserControl
    {
        public RemoveProgressPanel()
        {
            InitializeComponent();
        }

        public void Remove()
        {
            string install_path = Program.InstallPath;

            this.Update();
            this.Invalidate();
            this.Refresh();

            int prog_total = 3 + Program.FileNames.Length + 3;
            int i = 0;

            ++i;
            progressBar.Value = 100 * i / prog_total;

            Program.RemoveShortcuts();

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            Program.StopService(false);

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            Program.UninstallService(false);

            this.Update();
            this.Invalidate();
            this.Refresh();

            bool in_use = false;

            foreach (var name in Program.FileNames)
            {
                ++i;
                progressBar.Value = 100 * i / prog_total;

                string dest_file = Path.Combine(install_path, name);

                Console.WriteLine("Removing file: " + dest_file);

                try
                {
                    if (File.Exists(dest_file))
                    {
                        File.Delete(dest_file);
                    }
                }
                catch (Exception)
                {
                    Console.WriteLine("File was in use: ", dest_file);
                    in_use = true;
                }

                this.Update();
                this.Invalidate();
                this.Refresh();
            }

            if (in_use)
            {
                Console.WriteLine("WARNING: One or more files were in use during removal");
            }

            ++i;
            progressBar.Value = 100 * i / prog_total;

            Program.RemoveUninstaller();

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            try
            {
                if (Directory.Exists(install_path))
                {
                    Directory.Delete(install_path);
                }
            }
            catch (Exception)
            {
                in_use = true;
            }

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            License.RevokeLicense();
            Program.RemoveInstallPath();

            this.Update();
            this.Invalidate();
            this.Refresh();

            Program.setup_form.remove_done_panel.Show();
            this.Hide();
        }
    }
}
