using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsUI
{
    partial class AboutBox : Form
    {
        public AboutBox()
        {
            InitializeComponent();

            //this.Text = String.Format("About {0}", AssemblyTitle);
            this.Text = "XRmonitors - About";
            this.labelProductName.Text = AssemblyProduct;
            this.labelVersion.Text = String.Format("Version {0}", AssemblyVersion);
            this.labelCopyright.Text = AssemblyCopyright;
            this.webToS.DocumentText =
@"
<h2>End-User License Agreement (EULA) of <span class=""app_name"">XRmonitors</span></h2>

<p> This End - User License Agreement(""EULA"") is a legal agreement between you and <span class=""company_name"">Augmented Perception Corporation </span></p>

<p>This EULA agreement governs your acquisition and use of our <span class=""app_name"">XRmonitors</span> software(""Software"") directly from <span class=""company_name"">Augmented Perception Corporation</span> or indirectly through a <span class=""company_name"">Augmented Perception Corporation</span> authorized reseller or distributor(a ""Reseller"").  </p>

<p>Please read this EULA agreement carefully before completing the installation process and using the <span class=""app_name"">XRmonitors</span> software.  It provides a license to use the <span class=""app_name"">XRmonitors</span> software and contains warranty information and liability disclaimers.  </p>

<p>If you register for a free trial of the <span class=""app_name"">XRmonitors</span> software, this EULA agreement will also govern that trial.  By clicking ""accept"" or installing and/or using the <span class=""app_name"">XRmonitors</span> software, you are confirming your acceptance of the Software and agreeing to become bound by the terms of this EULA agreement.  </p>

<p>If you are entering into this EULA agreement on behalf of a company or other legal entity, you represent that you have the authority to bind such entity and its affiliates to these terms and conditions.  If you do not have such authority or if you do not agree with the terms and conditions of this EULA agreement, do not install or use the Software, and you must not accept this EULA agreement.  </p>

<p>This EULA agreement shall apply only to the Software supplied by <span class=""company_name"">Augmented Perception Corporation</span> herewith regardless of whether other software is referred to or described herein.  The terms also apply to any <span class=""company_name"">Augmented Perception Corporation</span> updates, supplements, Internet-based services, and support services for the Software, unless other terms accompany those items on delivery.  If so, those terms apply.  </p>

<h3>License Grant</h3>

<p><span class=""company_name"">Augmented Perception Corporation</span> hereby grants you a personal, non-transferable, non-exclusive licence to use the<span class=""app_name"">XRmonitors</span> software on your devices in accordance with the terms of this EULA agreement.  </p>

<p>You are permitted to load the <span class=""app_name"">XRmonitors</span> software(for example a PC, laptop, mobile or tablet) under your control.  You are responsible for ensuring your device meets the minimum requirements of the <span class=""app_name"">XRmonitors</span> software.  </p>

<p>You are not permitted to:</p>

<ul>
<li>Edit, alter, modify, adapt, translate or otherwise change the whole or any part of the Software nor permit the whole or any part of the Software to be combined with or become incorporated in any other software, nor decompile, disassemble or reverse engineer the Software or attempt to do any such things.</li>
<li>Reproduce, copy, distribute, resell or otherwise use the Software for any commercial purpose.</li>
<li>Allow any third party to use the Software on behalf of or for the benefit of any third party.</li>
<li>Use the Software in any way which breaches any applicable local, national or international law.</li>
<li>Use the Software for any purpose that <span class=""company_name"">Augmented Perception Corporation</span> considers is a breach of this EULA agreement.</li>
</ul>

<h3>Intellectual Property and Ownership</h3>

<p><span class=""company_name"">Augmented Perception Corporation</span> shall at all times retain ownership of the Software as originally downloaded by you and all subsequent downloads of the Software by you.  The Software(and the copyright, and other intellectual property rights of whatever nature in the Software, including any modifications made thereto) are and shall remain the property of<span class=""company_name"">Augmented Perception Corporation</span>.  </p>

<p><span class=""company_name"">Augmented Perception Corporation</span> reserves the right to grant licences to use the Software to third parties.  </p>

<h3>Termination</h3>

<p>This EULA agreement is effective from the date you first use the Software and shall continue until terminated.  You may terminate it at any time upon written notice to<span class=""company_name"">Augmented Perception Corporation</span>.  </p>

<p>It will also terminate immediately if you fail to comply with any term of this EULA agreement.  Upon such termination, the licenses granted by this EULA agreement will immediately terminate and you agree to stop all access and use of the Software.  The provisions that by their nature continue and survive will survive any termination of this EULA agreement.  </p>

<h3>Governing Law</h3>

<p>This EULA agreement, and any dispute arising out of or in connection with this EULA agreement, shall be governed by and construed in accordance with the laws of <span class=""country"">The United States of America</span>.  </p>
";
            this.webPriv.DocumentText =
@"
<h2>Privacy Policy</h2>
<p>Your privacy is important to us. It is Augmented Perception Corporation's policy to respect your privacy regarding any information we may collect from you across our software, our website, http://xrmonitors.com, and other sites we own and operate.</p>
<p>We only ask for personal information when we truly need it to provide a service to you. We collect it by fair and lawful means, with your knowledge and consent. We also let you know why we’re collecting it and how it will be used.</p>
<p>We only retain collected information for as long as necessary to provide you with your requested service. What data we store, we’ll protect within commercially acceptable means to prevent loss and theft, as well as unauthorised access, disclosure, copying, use or modification.</p>
<p>We don’t share any personally identifying information publicly or with third-parties, except when required to by law.</p>
<p>Our website may link to external sites that are not operated by us. Please be aware that we have no control over the content and practices of these sites, and cannot accept responsibility or liability for their respective privacy policies.</p>
<p>You are free to refuse our request for your personal information, with the understanding that we may be unable to provide you with some of your desired services.</p>
<p>Your continued use of our software will be regarded as acceptance of our practices around privacy and personal information. If you have any questions about how we handle user data and personal information, feel free to contact us at support@xrmonitors.com.</p>
<p>This policy is effective as of 11 May 2019.</p>
";

            this.labelLicense.Text = "OPEN SOURCE VERSION";
        }

        #region Assembly Attribute Accessors

        public string AssemblyTitle
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyTitleAttribute), false);
                if (attributes.Length > 0)
                {
                    AssemblyTitleAttribute titleAttribute = (AssemblyTitleAttribute)attributes[0];
                    if (titleAttribute.Title != "")
                    {
                        return titleAttribute.Title;
                    }
                }
                return System.IO.Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().CodeBase);
            }
        }

        public string AssemblyVersion
        {
            get
            {
                return Assembly.GetExecutingAssembly().GetName().Version.ToString();
            }
        }

        public string AssemblyDescription
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyDescriptionAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyDescriptionAttribute)attributes[0]).Description;
            }
        }

        public string AssemblyProduct
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyProductAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyProductAttribute)attributes[0]).Product;
            }
        }

        public string AssemblyCopyright
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCopyrightAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyCopyrightAttribute)attributes[0]).Copyright;
            }
        }

        public string AssemblyCompany
        {
            get
            {
                object[] attributes = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(AssemblyCompanyAttribute), false);
                if (attributes.Length == 0)
                {
                    return "";
                }
                return ((AssemblyCompanyAttribute)attributes[0]).Company;
            }
        }
        #endregion

        private void RevokeLicenseToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("License features disabled for open source distribution");
        }
    }
}
