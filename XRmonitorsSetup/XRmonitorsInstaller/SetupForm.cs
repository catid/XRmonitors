using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsInstaller
{
    public partial class SetupForm : Form
    {
        public XRmonitorsInstaller.WelcomePanel welcome_panel = new XRmonitorsInstaller.WelcomePanel();
        public XRmonitorsInstaller.LegalPanel legal_panel = new XRmonitorsInstaller.LegalPanel();
        public XRmonitorsInstaller.LicensePanel license_panel = new XRmonitorsInstaller.LicensePanel();
        public XRmonitorsInstaller.InstallPanel install_panel = new XRmonitorsInstaller.InstallPanel();
        public XRmonitorsInstaller.ProgressPanel progress_panel = new XRmonitorsInstaller.ProgressPanel();
        public XRmonitorsInstaller.CompletePanel complete_panel = new XRmonitorsInstaller.CompletePanel();
        public XRmonitorsInstaller.FailedPanel failed_panel = new XRmonitorsInstaller.FailedPanel();

        public XRmonitorsInstaller.RemovePanel remove_panel = new XRmonitorsInstaller.RemovePanel();
        public XRmonitorsInstaller.RemoveProgressPanel remove_progress_panel = new XRmonitorsInstaller.RemoveProgressPanel();
        public XRmonitorsInstaller.RemoveDonePanel remove_done_panel = new XRmonitorsInstaller.RemoveDonePanel();
        public XRmonitorsInstaller.RemoveFailedPanel remove_failed_panel = new XRmonitorsInstaller.RemoveFailedPanel();

        public SetupForm()
        {
            InitializeComponent();

            welcome_panel.Hide();
            legal_panel.Hide();
            license_panel.Hide();
            install_panel.Hide();
            progress_panel.Hide();
            complete_panel.Hide();
            failed_panel.Hide();

            remove_panel.Hide();
            remove_progress_panel.Hide();
            remove_done_panel.Hide();
            remove_failed_panel.Hide();

            this.panelContainer.Controls.Add(welcome_panel);
            this.panelContainer.Controls.Add(legal_panel);
            this.panelContainer.Controls.Add(license_panel);
            this.panelContainer.Controls.Add(install_panel);
            this.panelContainer.Controls.Add(progress_panel);
            this.panelContainer.Controls.Add(complete_panel);
            this.panelContainer.Controls.Add(failed_panel);

            this.panelContainer.Controls.Add(remove_panel);
            this.panelContainer.Controls.Add(remove_progress_panel);
            this.panelContainer.Controls.Add(remove_done_panel);
            this.panelContainer.Controls.Add(remove_failed_panel);

            if (Program.Register)
            {
                license_panel.Show();
            }
            else if (Program.Uninstalling)
            {
                if (Program.IsInstalled)
                {
                    remove_panel.Show();
                }
                else
                {
                    MessageBox.Show("XRmonitors is already uninstalled");
                    Application.Exit();
                }
            }
            else if (Program.Upgrading)
            {
                if (Program.IsInstalled)
                {
                    welcome_panel.Show();
                }
                else
                {
                    // Weird but okay - Let's install
                    welcome_panel.Show();
                }
            }
            else
            {
                if (Program.IsInstalled)
                {
                    remove_panel.Show();
                }
                else
                {
                    welcome_panel.Show();
                }
            }
        }
    }
}
