using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

namespace XRmonitorsUI
{
    public partial class DemoExpired : Form
    {
        public DemoExpired()
        {
            InitializeComponent();
        }

        private void DemoExpired_Load(object sender, EventArgs e)
        {
            webBrowser1.DocumentText =
@"
<h1>Your trial period has ended.</h1>  

<h2>We are glad you have found <span class=""app_name"">XRmonitors</span> useful for your professional work.</h2>  

<h2>Please consider purchasing the software on our website to continue using XRmonitors.  Look out for sales or referrals to get the best deal.</h2>
";
        }

        private void Button2_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("https://xrmonitors.com");
        }

        private void Button1_Click(object sender, EventArgs e)
        {
            if (textBox1.Text.Length > 0)
            {
                if (AltPcgi.SetSerialNumber(textBox1.Text.Trim()) != (int)AltPcgi.ReturnCodes.PCGI_STATUS_OK)
                {
                    MessageBox.Show("Please check the license key.");
                    return;
                }

                Console.WriteLine("Launching installer");

                AltPcgi.InvalidateSerialNumber();

                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = Path.Combine(Program.InstallPath, "XRmonitorsSetup.exe");
                psi.Arguments = "/register " + textBox1.Text.Trim();
                Process process = Process.Start(psi);

                Console.WriteLine("Launched");
            }

            Application.Exit();
        }

        private void FixSerialBox()
        {
            int select_start = textBox1.SelectionStart;

            String text = textBox1.Text;
            String result = "";
            foreach (char c in text.ToCharArray())
            {
                if ((c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z'))
                {
                    if (result.Length == 4 ||
                        result.Length == 9 ||
                        result.Length == 12 ||
                        result.Length == 17)
                    {
                        result += "-";
                    }
                    else if (result.Length >= 22)
                    {
                        break;
                    }

                    result += char.ToUpper(c);
                }
            }

            if (text != result)
            {
                textBox1.Text = result;
                if (select_start + 1 >= text.Length)
                {
                    textBox1.SelectionStart = result.Length;
                }
                else
                {
                    textBox1.SelectionStart = select_start;
                }
            }
        }

        private void TextBox1_TextChanged(object sender, EventArgs e)
        {
            if (textBox1.Text.Length > 0)
            {
                button1.Text = "Apply Key";
                FixSerialBox();
            }
            else
            {
                button1.Text = "Exit";
            }
        }
    }
}
