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
    public partial class ReportIssue : Form
    {
        public ReportIssue()
        {
            InitializeComponent();
        }

        private void LinkLabelIssueTracker_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkIssueTracker.Text;
            Process.Start(psi);
        }

        private void LinkLabelTwitter_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkLabelTwitter.Text;
            Process.Start(psi);
        }

        private void LinkLabelSlack_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            var psi = new ProcessStartInfo();
            psi.UseShellExecute = true;
            psi.FileName = linkLabelSlack.Text;
            Process.Start(psi);
        }

        private void Button1_Click(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}
