namespace XRmonitorsUI
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.groupWMR = new System.Windows.Forms.GroupBox();
            this.label3 = new System.Windows.Forms.Label();
            this.buttonXr = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.buttonLaunchPortal = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.buttonWinSettings = new System.Windows.Forms.Button();
            this.groupVMGo = new System.Windows.Forms.GroupBox();
            this.panelEnable = new System.Windows.Forms.Panel();
            this.groupVMSettings = new System.Windows.Forms.GroupBox();
            this.groupBox7 = new System.Windows.Forms.GroupBox();
            this.panelPanRight = new System.Windows.Forms.Panel();
            this.buttonRebindPanRight = new System.Windows.Forms.Button();
            this.checkBlue = new System.Windows.Forms.CheckBox();
            this.groupBox5 = new System.Windows.Forms.GroupBox();
            this.panelDecrease = new System.Windows.Forms.Panel();
            this.buttonRebindDecrease = new System.Windows.Forms.Button();
            this.groupBox4 = new System.Windows.Forms.GroupBox();
            this.panelIncrease = new System.Windows.Forms.Panel();
            this.buttonRebindIncrease = new System.Windows.Forms.Button();
            this.groupBox6 = new System.Windows.Forms.GroupBox();
            this.panelPanLeft = new System.Windows.Forms.Panel();
            this.buttonRebindPanLeft = new System.Windows.Forms.Button();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.panelAltDecrease = new System.Windows.Forms.Panel();
            this.buttonRebindAltDecrease = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.panelAltIncrease = new System.Windows.Forms.Panel();
            this.buttonRebindAltIncrease = new System.Windows.Forms.Button();
            this.groupBox3 = new System.Windows.Forms.GroupBox();
            this.panelRecenter = new System.Windows.Forms.Panel();
            this.buttonRebindRecenter = new System.Windows.Forms.Button();
            this.checkEnablePassthrough = new System.Windows.Forms.CheckBox();
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.reportIssueToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.disableWinY = new System.Windows.Forms.ToolStripMenuItem();
            this.checkAutoStart = new System.Windows.Forms.ToolStripMenuItem();
            this.showVMSettings = new System.Windows.Forms.ToolStripMenuItem();
            this.showWMR = new System.Windows.Forms.ToolStripMenuItem();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.flowLayoutPanel1.SuspendLayout();
            this.groupWMR.SuspendLayout();
            this.groupVMGo.SuspendLayout();
            this.groupVMSettings.SuspendLayout();
            this.groupBox7.SuspendLayout();
            this.groupBox5.SuspendLayout();
            this.groupBox4.SuspendLayout();
            this.groupBox6.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox3.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoScroll = true;
            this.flowLayoutPanel1.AutoSize = true;
            this.flowLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel1.Controls.Add(this.groupWMR);
            this.flowLayoutPanel1.Controls.Add(this.groupVMGo);
            this.flowLayoutPanel1.Controls.Add(this.groupVMSettings);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Padding = new System.Windows.Forms.Padding(0, 40, 0, 0);
            this.flowLayoutPanel1.Size = new System.Drawing.Size(889, 978);
            this.flowLayoutPanel1.TabIndex = 0;
            this.flowLayoutPanel1.WrapContents = false;
            // 
            // groupWMR
            // 
            this.groupWMR.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.groupWMR.Controls.Add(this.label3);
            this.groupWMR.Controls.Add(this.buttonXr);
            this.groupWMR.Controls.Add(this.label2);
            this.groupWMR.Controls.Add(this.buttonLaunchPortal);
            this.groupWMR.Controls.Add(this.label1);
            this.groupWMR.Controls.Add(this.buttonWinSettings);
            this.groupWMR.Location = new System.Drawing.Point(20, 59);
            this.groupWMR.Margin = new System.Windows.Forms.Padding(20, 19, 20, 19);
            this.groupWMR.Name = "groupWMR";
            this.groupWMR.Padding = new System.Windows.Forms.Padding(20, 19, 20, 19);
            this.groupWMR.Size = new System.Drawing.Size(809, 288);
            this.groupWMR.TabIndex = 0;
            this.groupWMR.TabStop = false;
            this.groupWMR.Text = "Windows Mixed Reality";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(213, 213);
            this.label3.Margin = new System.Windows.Forms.Padding(8, 0, 8, 0);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(481, 39);
            this.label3.TabIndex = 5;
            this.label3.Text = "Launch OpenXR Developer Tools";
            // 
            // buttonXr
            // 
            this.buttonXr.Location = new System.Drawing.Point(28, 203);
            this.buttonXr.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.buttonXr.Name = "buttonXr";
            this.buttonXr.Size = new System.Drawing.Size(160, 60);
            this.buttonXr.TabIndex = 4;
            this.buttonXr.Text = "OpenXR";
            this.buttonXr.UseVisualStyleBackColor = true;
            this.buttonXr.Click += new System.EventHandler(this.ButtonXr_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(215, 141);
            this.label2.Margin = new System.Windows.Forms.Padding(8, 0, 8, 0);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(546, 39);
            this.label2.TabIndex = 3;
            this.label2.Text = "Launch Windows Mixed Reality Portal";
            // 
            // buttonLaunchPortal
            // 
            this.buttonLaunchPortal.Location = new System.Drawing.Point(29, 131);
            this.buttonLaunchPortal.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.buttonLaunchPortal.Name = "buttonLaunchPortal";
            this.buttonLaunchPortal.Size = new System.Drawing.Size(160, 60);
            this.buttonLaunchPortal.TabIndex = 2;
            this.buttonLaunchPortal.Text = "Portal";
            this.buttonLaunchPortal.UseVisualStyleBackColor = true;
            this.buttonLaunchPortal.Click += new System.EventHandler(this.ButtonLaunchPortal_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(208, 69);
            this.label1.Margin = new System.Windows.Forms.Padding(8, 0, 8, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(515, 39);
            this.label1.TabIndex = 1;
            this.label1.Text = "Settings for Windows Mixed Reality";
            // 
            // buttonWinSettings
            // 
            this.buttonWinSettings.Location = new System.Drawing.Point(29, 60);
            this.buttonWinSettings.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.buttonWinSettings.Name = "buttonWinSettings";
            this.buttonWinSettings.Size = new System.Drawing.Size(160, 60);
            this.buttonWinSettings.TabIndex = 0;
            this.buttonWinSettings.Text = "Settings";
            this.buttonWinSettings.UseVisualStyleBackColor = true;
            this.buttonWinSettings.Click += new System.EventHandler(this.ButtonWinSettings_Click);
            // 
            // groupVMGo
            // 
            this.groupVMGo.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.groupVMGo.Controls.Add(this.panelEnable);
            this.groupVMGo.Location = new System.Drawing.Point(20, 385);
            this.groupVMGo.Margin = new System.Windows.Forms.Padding(20, 19, 20, 19);
            this.groupVMGo.Name = "groupVMGo";
            this.groupVMGo.Padding = new System.Windows.Forms.Padding(20, 19, 20, 19);
            this.groupVMGo.Size = new System.Drawing.Size(703, 160);
            this.groupVMGo.TabIndex = 2;
            this.groupVMGo.TabStop = false;
            this.groupVMGo.Text = "Virtual Monitors";
            // 
            // panelEnable
            // 
            this.panelEnable.Location = new System.Drawing.Point(36, 48);
            this.panelEnable.Margin = new System.Windows.Forms.Padding(4);
            this.panelEnable.Name = "panelEnable";
            this.panelEnable.Size = new System.Drawing.Size(643, 80);
            this.panelEnable.TabIndex = 5;
            // 
            // groupVMSettings
            // 
            this.groupVMSettings.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.groupVMSettings.Controls.Add(this.groupBox7);
            this.groupVMSettings.Controls.Add(this.checkBlue);
            this.groupVMSettings.Controls.Add(this.groupBox5);
            this.groupVMSettings.Controls.Add(this.groupBox4);
            this.groupVMSettings.Controls.Add(this.groupBox6);
            this.groupVMSettings.Controls.Add(this.groupBox2);
            this.groupVMSettings.Controls.Add(this.groupBox1);
            this.groupVMSettings.Controls.Add(this.groupBox3);
            this.groupVMSettings.Controls.Add(this.checkEnablePassthrough);
            this.groupVMSettings.Location = new System.Drawing.Point(20, 583);
            this.groupVMSettings.Margin = new System.Windows.Forms.Padding(20, 19, 20, 19);
            this.groupVMSettings.Name = "groupVMSettings";
            this.groupVMSettings.Padding = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.groupVMSettings.Size = new System.Drawing.Size(809, 1519);
            this.groupVMSettings.TabIndex = 1;
            this.groupVMSettings.TabStop = false;
            this.groupVMSettings.Text = "Virtual Monitors Settings";
            // 
            // groupBox7
            // 
            this.groupBox7.Controls.Add(this.panelPanRight);
            this.groupBox7.Controls.Add(this.buttonRebindPanRight);
            this.groupBox7.Location = new System.Drawing.Point(29, 1317);
            this.groupBox7.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox7.Name = "groupBox7";
            this.groupBox7.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox7.Size = new System.Drawing.Size(751, 171);
            this.groupBox7.TabIndex = 14;
            this.groupBox7.TabStop = false;
            this.groupBox7.Text = "Shortcut: Pan Right";
            // 
            // panelPanRight
            // 
            this.panelPanRight.Location = new System.Drawing.Point(192, 52);
            this.panelPanRight.Margin = new System.Windows.Forms.Padding(4);
            this.panelPanRight.Name = "panelPanRight";
            this.panelPanRight.Size = new System.Drawing.Size(532, 91);
            this.panelPanRight.TabIndex = 5;
            // 
            // buttonRebindPanRight
            // 
            this.buttonRebindPanRight.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindPanRight.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindPanRight.Name = "buttonRebindPanRight";
            this.buttonRebindPanRight.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindPanRight.TabIndex = 3;
            this.buttonRebindPanRight.Text = "Rebind";
            this.buttonRebindPanRight.UseVisualStyleBackColor = true;
            this.buttonRebindPanRight.Click += new System.EventHandler(this.ButtonRebindPanRight_Click);
            // 
            // checkBlue
            // 
            this.checkBlue.AutoSize = true;
            this.checkBlue.Location = new System.Drawing.Point(29, 107);
            this.checkBlue.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.checkBlue.Name = "checkBlue";
            this.checkBlue.Size = new System.Drawing.Size(582, 43);
            this.checkBlue.TabIndex = 13;
            this.checkBlue.Text = "Ergonomics: Blue light reduction filter";
            this.checkBlue.UseVisualStyleBackColor = true;
            this.checkBlue.CheckedChanged += new System.EventHandler(this.CheckBlue_CheckedChanged);
            // 
            // groupBox5
            // 
            this.groupBox5.Controls.Add(this.panelDecrease);
            this.groupBox5.Controls.Add(this.buttonRebindDecrease);
            this.groupBox5.Location = new System.Drawing.Point(29, 549);
            this.groupBox5.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox5.Name = "groupBox5";
            this.groupBox5.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox5.Size = new System.Drawing.Size(751, 171);
            this.groupBox5.TabIndex = 12;
            this.groupBox5.TabStop = false;
            this.groupBox5.Text = "Shortcut: Decrease Size";
            // 
            // panelDecrease
            // 
            this.panelDecrease.Location = new System.Drawing.Point(192, 52);
            this.panelDecrease.Margin = new System.Windows.Forms.Padding(4);
            this.panelDecrease.Name = "panelDecrease";
            this.panelDecrease.Size = new System.Drawing.Size(532, 91);
            this.panelDecrease.TabIndex = 5;
            // 
            // buttonRebindDecrease
            // 
            this.buttonRebindDecrease.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindDecrease.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindDecrease.Name = "buttonRebindDecrease";
            this.buttonRebindDecrease.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindDecrease.TabIndex = 3;
            this.buttonRebindDecrease.Text = "Rebind";
            this.buttonRebindDecrease.UseVisualStyleBackColor = true;
            this.buttonRebindDecrease.Click += new System.EventHandler(this.ButtonRebindDecrease_Click);
            // 
            // groupBox4
            // 
            this.groupBox4.Controls.Add(this.panelIncrease);
            this.groupBox4.Controls.Add(this.buttonRebindIncrease);
            this.groupBox4.Location = new System.Drawing.Point(29, 360);
            this.groupBox4.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox4.Name = "groupBox4";
            this.groupBox4.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox4.Size = new System.Drawing.Size(751, 171);
            this.groupBox4.TabIndex = 11;
            this.groupBox4.TabStop = false;
            this.groupBox4.Text = "Shortcut: Increase Size";
            // 
            // panelIncrease
            // 
            this.panelIncrease.Location = new System.Drawing.Point(192, 52);
            this.panelIncrease.Margin = new System.Windows.Forms.Padding(4);
            this.panelIncrease.Name = "panelIncrease";
            this.panelIncrease.Size = new System.Drawing.Size(532, 91);
            this.panelIncrease.TabIndex = 5;
            // 
            // buttonRebindIncrease
            // 
            this.buttonRebindIncrease.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindIncrease.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindIncrease.Name = "buttonRebindIncrease";
            this.buttonRebindIncrease.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindIncrease.TabIndex = 3;
            this.buttonRebindIncrease.Text = "Rebind";
            this.buttonRebindIncrease.UseVisualStyleBackColor = true;
            this.buttonRebindIncrease.Click += new System.EventHandler(this.ButtonRebindIncrease_Click);
            // 
            // groupBox6
            // 
            this.groupBox6.Controls.Add(this.panelPanLeft);
            this.groupBox6.Controls.Add(this.buttonRebindPanLeft);
            this.groupBox6.Location = new System.Drawing.Point(29, 1125);
            this.groupBox6.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox6.Name = "groupBox6";
            this.groupBox6.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox6.Size = new System.Drawing.Size(751, 171);
            this.groupBox6.TabIndex = 13;
            this.groupBox6.TabStop = false;
            this.groupBox6.Text = "Shortcut: Pan Left";
            // 
            // panelPanLeft
            // 
            this.panelPanLeft.Location = new System.Drawing.Point(192, 52);
            this.panelPanLeft.Margin = new System.Windows.Forms.Padding(4);
            this.panelPanLeft.Name = "panelPanLeft";
            this.panelPanLeft.Size = new System.Drawing.Size(532, 91);
            this.panelPanLeft.TabIndex = 5;
            // 
            // buttonRebindPanLeft
            // 
            this.buttonRebindPanLeft.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindPanLeft.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindPanLeft.Name = "buttonRebindPanLeft";
            this.buttonRebindPanLeft.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindPanLeft.TabIndex = 3;
            this.buttonRebindPanLeft.Text = "Rebind";
            this.buttonRebindPanLeft.UseVisualStyleBackColor = true;
            this.buttonRebindPanLeft.Click += new System.EventHandler(this.ButtonRebindPanLeft_Click);
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.panelAltDecrease);
            this.groupBox2.Controls.Add(this.buttonRebindAltDecrease);
            this.groupBox2.Location = new System.Drawing.Point(28, 933);
            this.groupBox2.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox2.Size = new System.Drawing.Size(751, 171);
            this.groupBox2.TabIndex = 13;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Shortcut: Alternative Decrease Size";
            // 
            // panelAltDecrease
            // 
            this.panelAltDecrease.Location = new System.Drawing.Point(192, 52);
            this.panelAltDecrease.Margin = new System.Windows.Forms.Padding(4);
            this.panelAltDecrease.Name = "panelAltDecrease";
            this.panelAltDecrease.Size = new System.Drawing.Size(532, 91);
            this.panelAltDecrease.TabIndex = 5;
            // 
            // buttonRebindAltDecrease
            // 
            this.buttonRebindAltDecrease.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindAltDecrease.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindAltDecrease.Name = "buttonRebindAltDecrease";
            this.buttonRebindAltDecrease.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindAltDecrease.TabIndex = 3;
            this.buttonRebindAltDecrease.Text = "Rebind";
            this.buttonRebindAltDecrease.UseVisualStyleBackColor = true;
            this.buttonRebindAltDecrease.Click += new System.EventHandler(this.ButtonRebindAltDecrease_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.panelAltIncrease);
            this.groupBox1.Controls.Add(this.buttonRebindAltIncrease);
            this.groupBox1.Location = new System.Drawing.Point(28, 741);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox1.Size = new System.Drawing.Size(751, 171);
            this.groupBox1.TabIndex = 13;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Shortcut: Alternative Incease Size";
            // 
            // panelAltIncrease
            // 
            this.panelAltIncrease.Location = new System.Drawing.Point(192, 52);
            this.panelAltIncrease.Margin = new System.Windows.Forms.Padding(4);
            this.panelAltIncrease.Name = "panelAltIncrease";
            this.panelAltIncrease.Size = new System.Drawing.Size(532, 91);
            this.panelAltIncrease.TabIndex = 5;
            // 
            // buttonRebindAltIncrease
            // 
            this.buttonRebindAltIncrease.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindAltIncrease.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindAltIncrease.Name = "buttonRebindAltIncrease";
            this.buttonRebindAltIncrease.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindAltIncrease.TabIndex = 3;
            this.buttonRebindAltIncrease.Text = "Rebind";
            this.buttonRebindAltIncrease.UseVisualStyleBackColor = true;
            this.buttonRebindAltIncrease.Click += new System.EventHandler(this.ButtonRebindAltIncrease_Click);
            // 
            // groupBox3
            // 
            this.groupBox3.Controls.Add(this.panelRecenter);
            this.groupBox3.Controls.Add(this.buttonRebindRecenter);
            this.groupBox3.Location = new System.Drawing.Point(29, 171);
            this.groupBox3.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Padding = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.groupBox3.Size = new System.Drawing.Size(751, 171);
            this.groupBox3.TabIndex = 10;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Shortcut: Recenter Monitors";
            // 
            // panelRecenter
            // 
            this.panelRecenter.Location = new System.Drawing.Point(192, 52);
            this.panelRecenter.Margin = new System.Windows.Forms.Padding(4);
            this.panelRecenter.Name = "panelRecenter";
            this.panelRecenter.Size = new System.Drawing.Size(532, 91);
            this.panelRecenter.TabIndex = 4;
            // 
            // buttonRebindRecenter
            // 
            this.buttonRebindRecenter.Location = new System.Drawing.Point(20, 60);
            this.buttonRebindRecenter.Margin = new System.Windows.Forms.Padding(8, 11, 8, 11);
            this.buttonRebindRecenter.Name = "buttonRebindRecenter";
            this.buttonRebindRecenter.Size = new System.Drawing.Size(160, 76);
            this.buttonRebindRecenter.TabIndex = 3;
            this.buttonRebindRecenter.Text = "Rebind";
            this.buttonRebindRecenter.UseVisualStyleBackColor = true;
            this.buttonRebindRecenter.Click += new System.EventHandler(this.ButtonRebindRecenter_Click);
            // 
            // checkEnablePassthrough
            // 
            this.checkEnablePassthrough.AutoSize = true;
            this.checkEnablePassthrough.Checked = true;
            this.checkEnablePassthrough.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkEnablePassthrough.Location = new System.Drawing.Point(29, 48);
            this.checkEnablePassthrough.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.checkEnablePassthrough.Name = "checkEnablePassthrough";
            this.checkEnablePassthrough.Size = new System.Drawing.Size(456, 43);
            this.checkEnablePassthrough.TabIndex = 0;
            this.checkEnablePassthrough.Text = "Enable camera pass-through";
            this.checkEnablePassthrough.UseVisualStyleBackColor = true;
            this.checkEnablePassthrough.CheckedChanged += new System.EventHandler(this.EnablePassthrough_CheckedChanged);
            // 
            // statusStrip1
            // 
            this.statusStrip1.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStrip1.Location = new System.Drawing.Point(0, 978);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Padding = new System.Windows.Forms.Padding(0, 0, 13, 0);
            this.statusStrip1.Size = new System.Drawing.Size(889, 22);
            this.statusStrip1.TabIndex = 2;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(0, 0);
            // 
            // menuStrip1
            // 
            this.menuStrip1.GripMargin = new System.Windows.Forms.Padding(2, 2, 0, 2);
            this.menuStrip1.ImageScalingSize = new System.Drawing.Size(32, 32);
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.settingsToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Margin = new System.Windows.Forms.Padding(3);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Padding = new System.Windows.Forms.Padding(0);
            this.menuStrip1.Size = new System.Drawing.Size(889, 36);
            this.menuStrip1.TabIndex = 4;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem,
            this.reportIssueToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(85, 36);
            this.fileToolStripMenuItem.Text = "Help";
            // 
            // aboutToolStripMenuItem
            // 
            this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
            this.aboutToolStripMenuItem.Size = new System.Drawing.Size(279, 44);
            this.aboutToolStripMenuItem.Text = "About";
            this.aboutToolStripMenuItem.Click += new System.EventHandler(this.AboutToolStripMenuItem_Click_1);
            // 
            // reportIssueToolStripMenuItem
            // 
            this.reportIssueToolStripMenuItem.Name = "reportIssueToolStripMenuItem";
            this.reportIssueToolStripMenuItem.Size = new System.Drawing.Size(279, 44);
            this.reportIssueToolStripMenuItem.Text = "Report Issue";
            this.reportIssueToolStripMenuItem.Click += new System.EventHandler(this.ReportIssueToolStripMenuItem_Click_1);
            // 
            // settingsToolStripMenuItem
            // 
            this.settingsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.disableWinY,
            this.checkAutoStart,
            this.showVMSettings,
            this.showWMR});
            this.settingsToolStripMenuItem.Name = "settingsToolStripMenuItem";
            this.settingsToolStripMenuItem.Size = new System.Drawing.Size(121, 36);
            this.settingsToolStripMenuItem.Text = "Settings";
            // 
            // disableWinY
            // 
            this.disableWinY.Checked = true;
            this.disableWinY.CheckOnClick = true;
            this.disableWinY.CheckState = System.Windows.Forms.CheckState.Checked;
            this.disableWinY.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.disableWinY.DoubleClickEnabled = true;
            this.disableWinY.Name = "disableWinY";
            this.disableWinY.Size = new System.Drawing.Size(595, 44);
            this.disableWinY.Text = "Automatically Disable WinKey + Y Prompt";
            this.disableWinY.CheckedChanged += new System.EventHandler(this.DisableWinY_CheckedChanged);
            // 
            // checkAutoStart
            // 
            this.checkAutoStart.Checked = true;
            this.checkAutoStart.CheckOnClick = true;
            this.checkAutoStart.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkAutoStart.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.checkAutoStart.DoubleClickEnabled = true;
            this.checkAutoStart.Name = "checkAutoStart";
            this.checkAutoStart.Size = new System.Drawing.Size(595, 44);
            this.checkAutoStart.Text = "Start Automatically when Windows Starts";
            this.checkAutoStart.CheckedChanged += new System.EventHandler(this.CheckAutoStart_CheckedChanged_1);
            // 
            // showVMSettings
            // 
            this.showVMSettings.Checked = true;
            this.showVMSettings.CheckOnClick = true;
            this.showVMSettings.CheckState = System.Windows.Forms.CheckState.Checked;
            this.showVMSettings.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.showVMSettings.DoubleClickEnabled = true;
            this.showVMSettings.Name = "showVMSettings";
            this.showVMSettings.Size = new System.Drawing.Size(595, 44);
            this.showVMSettings.Text = "Show: Virtual Monitors Settings";
            this.showVMSettings.CheckedChanged += new System.EventHandler(this.ShowVirtualMonitorsSettingsToolStripMenuItem_CheckedChanged);
            // 
            // showWMR
            // 
            this.showWMR.Checked = true;
            this.showWMR.CheckOnClick = true;
            this.showWMR.CheckState = System.Windows.Forms.CheckState.Checked;
            this.showWMR.DoubleClickEnabled = true;
            this.showWMR.Name = "showWMR";
            this.showWMR.Size = new System.Drawing.Size(595, 44);
            this.showWMR.Text = "Show: Windows Mixed Reality";
            this.showWMR.CheckedChanged += new System.EventHandler(this.ShowWindowsMixedRealityToolStripMenuItem_CheckedChanged);
            // 
            // timer1
            // 
            this.timer1.Interval = 2000;
            this.timer1.Tick += new System.EventHandler(this.Timer1_Tick);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(192F, 192F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(889, 1000);
            this.Controls.Add(this.menuStrip1);
            this.Controls.Add(this.flowLayoutPanel1);
            this.Controls.Add(this.statusStrip1);
            this.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.Margin = new System.Windows.Forms.Padding(8, 5, 8, 5);
            this.Name = "MainForm";
            this.Text = "XRmonitors";
            this.TopMost = true;
            this.FormClosed += new System.Windows.Forms.FormClosedEventHandler(this.MainForm_FormClosed);
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.Shown += new System.EventHandler(this.MainForm_Shown);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyDown);
            this.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.MainForm_KeyPress);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.MainForm_KeyUp);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.groupWMR.ResumeLayout(false);
            this.groupWMR.PerformLayout();
            this.groupVMGo.ResumeLayout(false);
            this.groupVMSettings.ResumeLayout(false);
            this.groupVMSettings.PerformLayout();
            this.groupBox7.ResumeLayout(false);
            this.groupBox5.ResumeLayout(false);
            this.groupBox4.ResumeLayout(false);
            this.groupBox6.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox3.ResumeLayout(false);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.GroupBox groupWMR;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button buttonLaunchPortal;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button buttonWinSettings;
        private System.Windows.Forms.GroupBox groupVMSettings;
        private System.Windows.Forms.CheckBox checkEnablePassthrough;
        private System.Windows.Forms.GroupBox groupBox3;
        private System.Windows.Forms.Button buttonRebindRecenter;
        private System.Windows.Forms.GroupBox groupBox5;
        private System.Windows.Forms.Button buttonRebindDecrease;
        private System.Windows.Forms.GroupBox groupBox4;
        private System.Windows.Forms.Button buttonRebindIncrease;
        private System.Windows.Forms.Panel panelDecrease;
        private System.Windows.Forms.Panel panelIncrease;
        private System.Windows.Forms.Panel panelRecenter;
        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem reportIssueToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem settingsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem disableWinY;
        private System.Windows.Forms.ToolStripMenuItem checkAutoStart;
        private System.Windows.Forms.GroupBox groupVMGo;
        private System.Windows.Forms.Panel panelEnable;
        private System.Windows.Forms.ToolStripMenuItem showVMSettings;
        private System.Windows.Forms.ToolStripMenuItem showWMR;
        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.CheckBox checkBlue;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button buttonXr;
        private System.Windows.Forms.GroupBox groupBox7;
        private System.Windows.Forms.Panel panelPanRight;
        private System.Windows.Forms.Button buttonRebindPanRight;
        private System.Windows.Forms.GroupBox groupBox6;
        private System.Windows.Forms.Panel panelPanLeft;
        private System.Windows.Forms.Button buttonRebindPanLeft;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Panel panelAltDecrease;
        private System.Windows.Forms.Button buttonRebindAltDecrease;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Panel panelAltIncrease;
        private System.Windows.Forms.Button buttonRebindAltIncrease;
    }
}