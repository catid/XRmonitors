using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsInstaller
{
    public partial class WelcomePanel : UserControl
    {
        public WelcomePanel()
        {
            InitializeComponent();
        }

        private void ButtonExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void ButtonStart_Click(object sender, EventArgs e)
        {
            if (Program.Upgrading && Program.IsInstalled)
            {
                // Upgrade immediately
                Program.setup_form.progress_panel.Show();
                this.Hide();
            }
            else if (Program.Uninstalling && Program.IsInstalled)
            {
                // Should not happen
                Program.setup_form.legal_panel.Show();
                this.Hide();
            }
            else
            {
                if (Program.IsInstalled)
                {
                    Program.setup_form.remove_panel.Show();
                }
                else
                {
                    Program.setup_form.legal_panel.Show();
                }
                this.Hide();
            }
        }
    }
}
