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
    public partial class RemovePanel : UserControl
    {
        public RemovePanel()
        {
            InitializeComponent();
        }

        private void ButtonExit_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        private void ButtonRemove_Click(object sender, EventArgs e)
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
                MessageBox.Show("An instance of XRmonitorsUI.exe is still running.  \n\nClose the software before removing it.", "XRmonitors - Still Running");
                return;
            }

            Program.setup_form.remove_progress_panel.Show();
            this.Hide();

            Program.setup_form.remove_progress_panel.Remove();
        }
    }
}
