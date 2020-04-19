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
    public partial class ProgressPanel : UserControl
    {
        public ProgressPanel()
        {
            InitializeComponent();
        }

        public static async Task CopyFileAsync(string sourceFile, string destinationFile, CancellationToken cancellationToken)
        {
            var fileOptions = FileOptions.Asynchronous | FileOptions.SequentialScan;
            var bufferSize = 81920;

            try
            {
                File.Delete(destinationFile);
            }
            catch (Exception e)
            {
                Console.WriteLine("Warning: " + destinationFile + " could not be deleted: " + e.ToString());
            }

            using (var sourceStream =
                  new FileStream(sourceFile, FileMode.Open, FileAccess.Read, FileShare.Read, bufferSize, fileOptions))

            using (var destinationStream =
                  new FileStream(destinationFile, FileMode.CreateNew, FileAccess.Write, FileShare.None, bufferSize, fileOptions))

                await sourceStream.CopyToAsync(destinationStream, bufferSize, cancellationToken)
                                           .ConfigureAwait(continueOnCapturedContext: false);
        }

        public void Install()
        {
            string install_path = Program.InstallPath;

            CancellationTokenSource source = new CancellationTokenSource();
            CancellationToken token = source.Token;

            int prog_total = 2 + Program.FileNames.Length + 5;
            int i = 0;

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

            foreach (var name in Program.FileNames)
            {
                ++i;
                progressBar.Value = 100 * i / prog_total;

                string source_file = Program.getTempResourcePath(name);
                string dest_file = Path.Combine(install_path, name);

                Console.WriteLine("Copy source: " + source_file);
                Console.WriteLine("Copy dest: " + dest_file);

                try
                {
                    CopyFileAsync(source_file, dest_file, token).Wait();
                }
                catch (Exception)
                {
                    MessageBox.Show("Installation destination folder is in use: " + dest_file + "  \n\nPlease select a different installation path and try again.   \n\nFor further assistance contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Install Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);

                    Program.setup_form.failed_panel.Show();
                    this.Hide();
                    return;
                }

                this.Update();
                this.Invalidate();
                this.Refresh();
            }

            ++i;
            progressBar.Value = 100 * i / prog_total;

            if (!Program.InstallService())
            {
                Program.setup_form.failed_panel.Show();
                this.Hide();
                return;
            }

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            if (!Program.StartService())
            {
                Program.setup_form.failed_panel.Show();
                this.Hide();
                return;
            }

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            if (!Program.CreateUninstaller())
            {
                Program.setup_form.failed_panel.Show();
                this.Hide();
                return;
            }

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            if (!Program.WriteInstallPath(install_path))
            {
                Program.setup_form.failed_panel.Show();
                this.Hide();
                return;
            }

            this.Update();
            this.Invalidate();
            this.Refresh();

            ++i;
            progressBar.Value = 100 * i / prog_total;

            Program.AddShortcuts();

            this.Update();
            this.Invalidate();
            this.Refresh();

            Program.setup_form.complete_panel.Show();
            this.Hide();
        }
    }
}
