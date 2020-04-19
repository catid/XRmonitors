using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Pipes;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;
using KeyEventArgs = System.Windows.Forms.KeyEventArgs;

namespace XRmonitorsUI
{
    public partial class MainForm : Form
    {
        private JCS.ToggleSwitch toggleEnableMonitors;

        public MainForm()
        {
            InitializeComponent();

            AddSlider();

            checkAutoStart.Checked = IsAutoStartEnabled();

            disableWinY.Checked = Properties.Settings.Default.DisableWinY;
            checkEnablePassthrough.Checked = Properties.Settings.Default.EnableCamera;
            showVMSettings.Checked = Properties.Settings.Default.ShowVMSettings;
            showWMR.Checked = Properties.Settings.Default.ShowWMR;
            checkBlue.Checked = Properties.Settings.Default.BlueLightFilter;

            //UpdateFormMinimumSize();

            Program.SetDisableWinY(disableWinY.Checked);
            Program.SetEnablePassthrough(checkEnablePassthrough.Checked);
            Program.SetBlueLightFilter(checkBlue.Checked);

            // Recenter:

            RecenterKeys.Clear();
            byte[] recenter_keys = System.Convert.FromBase64String(Properties.Settings.Default.RecenterKeys);
            if (recenter_keys != null && recenter_keys.Length > 0)
            {
                for (int i = 0; i < recenter_keys.Length; ++i)
                {
                    RecenterKeys.Add((Keys)recenter_keys[i]);
                }
            }
            else
            {
                RecenterKeys.Add(Keys.LControlKey);
                RecenterKeys.Add(Keys.Oemtilde);
            }
            UpdateRecenterKeys();

            // Increase:

            IncreaseKeys.Clear();
            byte[] increase_keys = System.Convert.FromBase64String(Properties.Settings.Default.IncreaseKeys);
            if (increase_keys != null && increase_keys.Length > 0)
            {
                for (int i = 0; i < increase_keys.Length; ++i)
                {
                    IncreaseKeys.Add((Keys)increase_keys[i]);
                }
            }
            else
            {
                IncreaseKeys.Add(Keys.RControlKey);
                IncreaseKeys.Add(Keys.PageUp);
            }
            UpdateIncreaseKeys();

            // Decrease:

            DecreaseKeys.Clear();
            byte[] decrease_keys = System.Convert.FromBase64String(Properties.Settings.Default.DecreaseKeys);
            if (decrease_keys != null && decrease_keys.Length > 0)
            {
                for (int i = 0; i < decrease_keys.Length; ++i)
                {
                    DecreaseKeys.Add((Keys)decrease_keys[i]);
                }
            }
            else
            {
                DecreaseKeys.Add(Keys.RControlKey);
                DecreaseKeys.Add(Keys.PageDown);
            }
            UpdateDecreaseKeys();

            // AltIncrease:

            AltIncreaseKeys.Clear();
            byte[] alt_increase_keys = System.Convert.FromBase64String(Properties.Settings.Default.AltIncreaseKeys);
            if (alt_increase_keys != null && alt_increase_keys.Length > 0)
            {
                for (int i = 0; i < alt_increase_keys.Length; ++i)
                {
                    AltIncreaseKeys.Add((Keys)alt_increase_keys[i]);
                }
            }
            else
            {
                AltIncreaseKeys.Add(Keys.LWin);
                AltIncreaseKeys.Add(Keys.LMenu);
                AltIncreaseKeys.Add(Keys.Up);
            }
            UpdateAltIncreaseKeys();

            // AltDecrease:

            AltDecreaseKeys.Clear();
            byte[] alt_decrease_keys = System.Convert.FromBase64String(Properties.Settings.Default.AltDecreaseKeys);
            if (alt_decrease_keys != null && alt_decrease_keys.Length > 0)
            {
                for (int i = 0; i < alt_decrease_keys.Length; ++i)
                {
                    AltDecreaseKeys.Add((Keys)alt_decrease_keys[i]);
                }
            }
            else
            {
                AltDecreaseKeys.Add(Keys.LWin);
                AltDecreaseKeys.Add(Keys.LMenu);
                AltDecreaseKeys.Add(Keys.Down);
            }
            UpdateAltDecreaseKeys();

            // PanLeft:

            PanLeftKeys.Clear();
            byte[] pan_left_keys = System.Convert.FromBase64String(Properties.Settings.Default.PanLeftKeys);
            if (pan_left_keys != null && pan_left_keys.Length > 0)
            {
                for (int i = 0; i < pan_left_keys.Length; ++i)
                {
                    PanLeftKeys.Add((Keys)pan_left_keys[i]);
                }
            }
            else
            {
                PanLeftKeys.Add(Keys.LWin);
                PanLeftKeys.Add(Keys.LMenu);
                PanLeftKeys.Add(Keys.Left);
            }
            UpdatePanLeftKeys();

            // PanRight:

            PanRightKeys.Clear();
            byte[] pan_right_keys = System.Convert.FromBase64String(Properties.Settings.Default.PanRightKeys);
            if (pan_right_keys != null && pan_right_keys.Length > 0)
            {
                for (int i = 0; i < pan_right_keys.Length; ++i)
                {
                    PanRightKeys.Add((Keys)pan_right_keys[i]);
                }
            }
            else
            {
                PanRightKeys.Add(Keys.LWin);
                PanRightKeys.Add(Keys.LMenu);
                PanRightKeys.Add(Keys.Right);
            }
            UpdatePanRightKeys();
        }

        private void AddSlider()
        {
            this.toggleEnableMonitors = new JCS.ToggleSwitch();

            this.panelEnable.Controls.Add(this.toggleEnableMonitors);
            // 
            // toggleEnableMonitors
            // 
            this.toggleEnableMonitors.Checked = true;
            this.toggleEnableMonitors.Dock = System.Windows.Forms.DockStyle.Fill;
            this.toggleEnableMonitors.Margin = new System.Windows.Forms.Padding(5, 3, 5, 3);
            this.toggleEnableMonitors.Name = "toggleEnableMonitors";
            this.toggleEnableMonitors.OffFont = new System.Drawing.Font("Microsoft Sans Serif", 7.875F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toggleEnableMonitors.OnFont = new System.Drawing.Font("Microsoft Sans Serif", 7.875F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toggleEnableMonitors.Size = new System.Drawing.Size(643, 80);
            this.toggleEnableMonitors.MaximumSize = new System.Drawing.Size(643, 80);
            this.toggleEnableMonitors.TabIndex = 2;
            this.toggleEnableMonitors.CheckedChanged += new JCS.ToggleSwitch.CheckedChangedDelegate(this.ToggleEnableMonitors_CheckedChanged);

            this.toggleEnableMonitors.Style = JCS.ToggleSwitch.ToggleSwitchStyle.Fancy;
            this.toggleEnableMonitors.OnText = "XR Monitors Enabled";
            this.toggleEnableMonitors.OnFont = new Font(this.toggleEnableMonitors.Font.FontFamily, 10, FontStyle.Bold);
            this.toggleEnableMonitors.OnForeColor = Color.White;
            this.toggleEnableMonitors.OffText = "XR Monitors Disabled";
            this.toggleEnableMonitors.OffFont = new Font(this.toggleEnableMonitors.Font.FontFamily, 10, FontStyle.Bold);
            this.toggleEnableMonitors.OffForeColor = Color.White;
            this.toggleEnableMonitors.ButtonImage = XRmonitorsUI.Properties.Resources.right_arrow;
            this.toggleEnableMonitors.ButtonScaleImageToFit = true;
        }

        public enum ActivateOptions
        {
            None = 0x00000000,  // No flags set
            DesignMode = 0x00000001,  // The application is being activated for design mode, and thus will not be able to
                                      // to create an immersive window. Window creation must be done by design tools which
                                      // load the necessary components by communicating with a designer-specified service on
                                      // the site chain established on the activation manager.  The splash screen normally
                                      // shown when an application is activated will also not appear.  Most activations
                                      // will not use this flag.
            NoErrorUI = 0x00000002,  // Do not show an error dialog if the app fails to activate.                                
            NoSplashScreen = 0x00000004,  // Do not show the splash screen when activating the app.
        }

        [ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        interface IApplicationActivationManager
        {
            // Activates the specified immersive application for the "Launch" contract, passing the provided arguments
            // string into the application.  Callers can obtain the process Id of the application instance fulfilling this contract.
            IntPtr ActivateApplication([In] String appUserModelId, [In] String arguments, [In] ActivateOptions options, [Out] out UInt32 processId);
            IntPtr ActivateForFile([In] String appUserModelId, [In] IntPtr /*IShellItemArray* */ itemArray, [In] String verb, [Out] out UInt32 processId);
            IntPtr ActivateForProtocol([In] String appUserModelId, [In] IntPtr /* IShellItemArray* */itemArray, [Out] out UInt32 processId);
        }

        [ComImport, Guid("45BA127D-10A8-46EA-8AB7-56EA9078943C")]//Application Activation Manager
        class ApplicationActivationManager : IApplicationActivationManager
        {
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)/*, PreserveSig*/]
            public extern IntPtr ActivateApplication([In] String appUserModelId, [In] String arguments, [In] ActivateOptions options, [Out] out UInt32 processId);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            public extern IntPtr ActivateForFile([In] String appUserModelId, [In] IntPtr /*IShellItemArray* */ itemArray, [In] String verb, [Out] out UInt32 processId);
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            public extern IntPtr ActivateForProtocol([In] String appUserModelId, [In] IntPtr /* IShellItemArray* */itemArray, [Out] out UInt32 processId);
        }

        private void ButtonWinSettings_Click(object sender, EventArgs e)
        {
            try
            {
                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = "ms-settings:holographic";
                Process.Start(psi);

                toolStripStatusLabel1.Text = "Launched: Windows Mixed Reality Settings";
            }
            catch (Exception)
            {
                toolStripStatusLabel1.Text = "Failed to launch: Windows Mixed Reality Settings";
                Program.g_OpenXrNeeded.Show(); this.Hide();
            }
        }

        private void ButtonLaunchPortal_Click(object sender, EventArgs e)
        {
            try
            {
                ApplicationActivationManager appActiveManager = new ApplicationActivationManager();//Class not registered
                uint pid;
                appActiveManager.ActivateApplication("Microsoft.MixedReality.Portal_8wekyb3d8bbwe!App", null, ActivateOptions.None, out pid);

                toolStripStatusLabel1.Text = "Launched: Windows Mixed Reality Portal";
            }
            catch (Exception)
            {
                toolStripStatusLabel1.Text = "Failed to launch: Windows Mixed Reality Portal";
                Program.g_OpenXrNeeded.Show(); this.Hide();
            }
        }

        private void ButtonXr_Click(object sender, EventArgs e)
        {
            // If developer version is installed prefer that one:
            if (Program.IsStoreAppInstalled("Microsoft.MixedRealityRuntimeDeveloperPreview"))
            {
                try
                {
                    ApplicationActivationManager appActiveManager = new ApplicationActivationManager();//Class not registered
                    uint pid;
                    appActiveManager.ActivateApplication("Microsoft.MixedRealityRuntimeDeveloperPreview_8wekyb3d8bbwe!App", null, ActivateOptions.None, out pid);

                    toolStripStatusLabel1.Text = "Launched: Mixed Reality Runtime Developer Preview";
                }
                catch (Exception)
                {
                    toolStripStatusLabel1.Text = "Failed to launch: Mixed Reality Runtime Developer Preview";
                    Program.g_OpenXrNeeded.Show(); this.Hide();
                }
            }
            // Else if user version is installed:
            else if (Program.IsStoreAppInstalled("Microsoft.WindowsMixedRealityRuntimeApp"))
            {
                toolStripStatusLabel1.Text = "Developer version of OpenXR is not installed";
            }
            else
            {
                toolStripStatusLabel1.Text = "Failed to launch: Mixed Reality OpenXR Runtime (not found)";
                Program.g_OpenXrNeeded.Show(); this.Hide();
            }
        }

        private void EnablePassthrough_CheckedChanged(object sender, EventArgs e)
        {
            Program.SetEnablePassthrough(checkEnablePassthrough.Checked);
            Properties.Settings.Default.EnableCamera = checkEnablePassthrough.Checked;

            if (checkEnablePassthrough.Checked)
            {
                toolStripStatusLabel1.Text = "Enabled: Pass-through Cameras will be displayed in VR.";
            }
            else
            {
                toolStripStatusLabel1.Text = "Disabled: Pass-through Cameras will NOT be displayed in VR.";
            }
        }


        /// <summary>
        /// Add application to Startup of windows
        /// </summary>
        /// <param name="appName"></param>
        /// <param name="path"></param>
        public static void AddStartup(string appName, string path)
        {
            using (RegistryKey key = Registry.CurrentUser.OpenSubKey
                ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true))
            {
                key.SetValue(appName, "\"" + path + "\"");
            }
        }

        public static string GetStartup(string appName)
        {
            using (RegistryKey key = Registry.CurrentUser.OpenSubKey
                ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true))
            {
                object value = key.GetValue(appName);
                if (value == null)
                {
                    return "";
                }
                string str = value as string;
                return str;
            }
        }

        /// <summary>
        /// Remove application from Startup of windows
        /// </summary>
        /// <param name="appName"></param>
        public static void RemoveStartup(string appName)
        {
            using (RegistryKey key = Registry.CurrentUser.OpenSubKey
                ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true))
            {
                key.DeleteValue(appName, false);
            }
        }

        private string FindExePath()
        {
            string location = System.Reflection.Assembly.GetEntryAssembly().Location;
            return location;
        }

        private bool IsAutoStartEnabled()
        {
            try
            {
                Console.WriteLine("Checking startup regkey");
                string path = FindExePath().Trim('"').Trim().Trim('"');
                Console.WriteLine("Found exe path: " + path);
                string actual_path = GetStartup("XRmonitors").Trim('"').Trim().Trim('"');

                if (0 == String.Compare(path.ToLower(), actual_path.ToLower()))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to set up AutoStart: ", ex.ToString());
                checkAutoStart.Enabled = false;
            }

            return false;
        }

        public bool VrStarted = false;
        public Process VrProc = null;

        private void StartVrProcess()
        {
            Program.SetEnableTerminated(false);

            VrStarted = true;

            if (VrProc != null)
            {
                return;
            }

            string app_name = "XRmonitorsHologram.exe";
            string app_path = Path.Combine(Program.InstallPath, app_name);

            VrProc = new Process
            {
                StartInfo = { FileName = app_path },
                EnableRaisingEvents = true
            };

            VrProc.Exited += (sender, args) =>
            {
                Invoke((MethodInvoker)delegate {
                    int exit_code = VrProc.ExitCode;

                    VrProc = null;

                    if (VrStarted)
                    {
                        if (exit_code != 0)
                        {
                            if (NeedsOpenXrSupport())
                            {
                                Program.g_OpenXrNeeded.Show(); this.Hide();
                                return;
                            }
                        }

                        toolStripStatusLabel1.Text = "Holographic Monitors exited (" + exit_code.ToString() + "); restarting...";

                        StartVrProcess();
                    }
                    else
                    {
                        toolStripStatusLabel1.Text = "Holographic Monitors exited (" + exit_code.ToString() + ").";
                    }
                });
            };

            try
            {
                VrProc.Start();
                toolStripStatusLabel1.Text = "Holographic Monitors running.";
            }
            catch (Exception)
            {
                toolStripStatusLabel1.Text = "Failed to start Holographic Monitors.";
            }
        }

        private bool NeedsOpenXrSupport()
        {
            int release_id = Program.GetReleaseId();
            if (release_id == 0)
            {
                // Cannot guess what to do
                return true;
            }

            if (release_id < 1809)
            {
                // Must upgrade to May 2019 update or newer
                return true;
            }
            else if (release_id < 1903)
            {
                // TBD: Do we need this anymore?
                if (!Program.IsProgramInstalled("Mixed Reality OpenXR Developer Preview Compatibility Pack"))
                {
                    // Missing the OpenXR Compatibility Pack
                    return true;
                }
                else
                {
                    // Should be good!
                }
            }
            else
            {
                // Should be good!
            }

            if (!Program.IsStoreAppInstalled("Microsoft.WindowsMixedRealityRuntimeApp") &&
                !Program.IsStoreAppInstalled("Microsoft.MixedRealityRuntimeDeveloperPreview"))
            {
                // Missing the OpenXR Runtime
                return true;
            }

            return false;
        }

        private void StopVrProcess()
        {
            Program.SetEnableTerminated(true);

            VrStarted = false;
            toolStripStatusLabel1.Text = "Holographic Monitors closing...";
        }

        private void ToggleEnableMonitors_CheckedChanged(object sender, EventArgs e)
        {
            if (this.toggleEnableMonitors.Checked)
            {
                StartVrProcess();
            }
            else
            {
                StopVrProcess();
            }
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            StartVrProcess();
        }


        //----------------------------------------------------------------------
        // Recenter

        private bool RebindingRecenter = false;
        private List<Keys> RecenterKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> RecenterPictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdateRecenterKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelRecenter, RecenterPictures);
            if (RebindingRecenter)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindRecenter.Text = "Cancel";
                }
                else
                {
                    buttonRebindRecenter.Text = "Accept";
                }
            }
        }

        private void UpdateRecenterKeys()
        {
            FilterKeys(RecenterKeys);
            UpdateRecenterKeyImages(RecenterKeys);

            byte[] recenter_keys = new byte[RecenterKeys.Count];
            for (int i = 0; i < RecenterKeys.Count; ++i)
            {
                recenter_keys[i] = (byte)RecenterKeys[i];
            }
            Properties.Settings.Default.RecenterKeys = System.Convert.ToBase64String(recenter_keys);

            Program.SetShortcutKeys(0, recenter_keys);
        }

        private void AcceptRecenter()
        {
            this.TopMost = false;

            RebindingRecenter = false;
            buttonRebindRecenter.Text = "Rebind";
            buttonRebindRecenter.Font = new Font(buttonRebindRecenter.Font.FontFamily, buttonRebindRecenter.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                RecenterKeys = new List<Keys>(PressedKeys);
            }

            UpdateRecenterKeys();

            toolStripStatusLabel1.Text = "Rebound: Recenter shortcut.";
        }

        private void ButtonRebindRecenter_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingRecenter)
            {
                AcceptRecenter();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingRecenter = true;
                buttonRebindRecenter.Text = "Cancel";
                buttonRebindRecenter.Font = new Font(buttonRebindRecenter.Font.FontFamily, buttonRebindRecenter.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Increase

        private bool RebindingIncrease = false;
        private List<Keys> IncreaseKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> IncreasePictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdateIncreaseKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelIncrease, IncreasePictures);
            if (RebindingIncrease)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindIncrease.Text = "Cancel";
                }
                else
                {
                    buttonRebindIncrease.Text = "Accept";
                }
            }
        }

        private void UpdateIncreaseKeys()
        {
            FilterKeys(IncreaseKeys);
            UpdateIncreaseKeyImages(IncreaseKeys);

            byte[] increase_keys = new byte[IncreaseKeys.Count];
            for (int i = 0; i < IncreaseKeys.Count; ++i)
            {
                increase_keys[i] = (byte)IncreaseKeys[i];
            }
            Properties.Settings.Default.IncreaseKeys = System.Convert.ToBase64String(increase_keys);

            Program.SetShortcutKeys(1, increase_keys);
        }

        private void AcceptIncrease()
        {
            this.TopMost = false;

            RebindingIncrease = false;
            buttonRebindIncrease.Text = "Rebind";
            buttonRebindIncrease.Font = new Font(buttonRebindIncrease.Font.FontFamily, buttonRebindIncrease.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                IncreaseKeys = new List<Keys>(PressedKeys);
            }

            UpdateIncreaseKeys();

            toolStripStatusLabel1.Text = "Rebound: Increase Monitor Size shortcut.";
        }

        private void ButtonRebindIncrease_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingIncrease)
            {
                AcceptIncrease();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingIncrease = true;
                buttonRebindIncrease.Text = "Cancel";
                buttonRebindIncrease.Font = new Font(buttonRebindIncrease.Font.FontFamily, buttonRebindIncrease.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Decrease

        private bool RebindingDecrease = false;
        private List<Keys> DecreaseKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> DecreasePictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdateDecreaseKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelDecrease, DecreasePictures);
            if (RebindingDecrease)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindDecrease.Text = "Cancel";
                }
                else
                {
                    buttonRebindDecrease.Text = "Accept";
                }
            }
        }

        void UpdateDecreaseKeys()
        {
            FilterKeys(DecreaseKeys);
            UpdateDecreaseKeyImages(DecreaseKeys);

            byte[] decrease_keys = new byte[DecreaseKeys.Count];
            for (int i = 0; i < DecreaseKeys.Count; ++i)
            {
                decrease_keys[i] = (byte)DecreaseKeys[i];
            }
            Properties.Settings.Default.DecreaseKeys = System.Convert.ToBase64String(decrease_keys);

            Program.SetShortcutKeys(2, decrease_keys);
        }

        private void AcceptDecrease()
        {
            this.TopMost = false;

            RebindingDecrease = false;
            buttonRebindDecrease.Text = "Rebind";
            buttonRebindDecrease.Font = new Font(buttonRebindDecrease.Font.FontFamily, buttonRebindDecrease.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                DecreaseKeys = new List<Keys>(PressedKeys);
            }

            UpdateDecreaseKeys();

            toolStripStatusLabel1.Text = "Rebound: Decrease Monitor Size shortcut.";
        }

        private void ButtonRebindDecrease_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingDecrease)
            {
                AcceptDecrease();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingDecrease = true;
                buttonRebindDecrease.Text = "Cancel";
                buttonRebindDecrease.Font = new Font(buttonRebindDecrease.Font.FontFamily, buttonRebindDecrease.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Alt Increase

        private bool RebindingAltIncrease = false;
        private List<Keys> AltIncreaseKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> AltIncreasePictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdateAltIncreaseKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelAltIncrease, AltIncreasePictures);
            if (RebindingAltIncrease)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindAltIncrease.Text = "Cancel";
                }
                else
                {
                    buttonRebindAltIncrease.Text = "Accept";
                }
            }
        }

        void UpdateAltIncreaseKeys()
        {
            FilterKeys(AltIncreaseKeys);
            UpdateAltIncreaseKeyImages(AltIncreaseKeys);

            byte[] keys = new byte[AltIncreaseKeys.Count];
            for (int i = 0; i < AltIncreaseKeys.Count; ++i)
            {
                keys[i] = (byte)AltIncreaseKeys[i];
            }
            Properties.Settings.Default.AltIncreaseKeys = System.Convert.ToBase64String(keys);

            Program.SetShortcutKeys(3, keys);
        }

        private void AcceptAltIncrease()
        {
            this.TopMost = false;

            RebindingAltIncrease = false;
            buttonRebindAltIncrease.Text = "Rebind";
            buttonRebindAltIncrease.Font = new Font(buttonRebindAltIncrease.Font.FontFamily, buttonRebindAltIncrease.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                AltIncreaseKeys = new List<Keys>(PressedKeys);
            }

            UpdateAltIncreaseKeys();

            toolStripStatusLabel1.Text = "Rebound: Alt Increase Size shortcut.";
        }

        private void ButtonRebindAltIncrease_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingAltIncrease)
            {
                AcceptAltIncrease();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingAltIncrease = true;
                buttonRebindAltIncrease.Text = "Cancel";
                buttonRebindAltIncrease.Font = new Font(buttonRebindAltIncrease.Font.FontFamily, buttonRebindAltIncrease.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Alt Decrease

        private bool RebindingAltDecrease = false;
        private List<Keys> AltDecreaseKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> AltDecreasePictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdateAltDecreaseKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelAltDecrease, AltDecreasePictures);
            if (RebindingAltDecrease)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindAltDecrease.Text = "Cancel";
                }
                else
                {
                    buttonRebindAltDecrease.Text = "Accept";
                }
            }
        }

        void UpdateAltDecreaseKeys()
        {
            FilterKeys(AltDecreaseKeys);
            UpdateAltDecreaseKeyImages(AltDecreaseKeys);

            byte[] keys = new byte[AltDecreaseKeys.Count];
            for (int i = 0; i < AltDecreaseKeys.Count; ++i)
            {
                keys[i] = (byte)AltDecreaseKeys[i];
            }
            Properties.Settings.Default.AltDecreaseKeys = System.Convert.ToBase64String(keys);

            Program.SetShortcutKeys(4, keys);
        }

        private void AcceptAltDecrease()
        {
            this.TopMost = false;

            RebindingAltDecrease = false;
            buttonRebindAltDecrease.Text = "Rebind";
            buttonRebindAltDecrease.Font = new Font(buttonRebindAltDecrease.Font.FontFamily, buttonRebindAltDecrease.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                AltDecreaseKeys = new List<Keys>(PressedKeys);
            }

            UpdateAltDecreaseKeys();

            toolStripStatusLabel1.Text = "Rebound: Alt Decrease Size shortcut.";
        }

        private void ButtonRebindAltDecrease_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingAltDecrease)
            {
                AcceptAltDecrease();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingAltDecrease = true;
                buttonRebindAltDecrease.Text = "Cancel";
                buttonRebindAltDecrease.Font = new Font(buttonRebindAltDecrease.Font.FontFamily, buttonRebindAltDecrease.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Pan Left

        private bool RebindingPanLeft = false;
        private List<Keys> PanLeftKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> PanLeftPictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdatePanLeftKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelPanLeft, PanLeftPictures);
            if (RebindingPanLeft)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindPanLeft.Text = "Cancel";
                }
                else
                {
                    buttonRebindPanLeft.Text = "Accept";
                }
            }
        }

        void UpdatePanLeftKeys()
        {
            FilterKeys(PanLeftKeys);
            UpdatePanLeftKeyImages(PanLeftKeys);

            byte[] keys = new byte[PanLeftKeys.Count];
            for (int i = 0; i < PanLeftKeys.Count; ++i)
            {
                keys[i] = (byte)PanLeftKeys[i];
            }
            Properties.Settings.Default.PanLeftKeys = System.Convert.ToBase64String(keys);

            Program.SetShortcutKeys(5, keys);
        }

        private void AcceptPanLeft()
        {
            this.TopMost = false;

            RebindingPanLeft = false;
            buttonRebindPanLeft.Text = "Rebind";
            buttonRebindPanLeft.Font = new Font(buttonRebindPanLeft.Font.FontFamily, buttonRebindPanLeft.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                PanLeftKeys = new List<Keys>(PressedKeys);
            }

            UpdatePanLeftKeys();

            toolStripStatusLabel1.Text = "Rebound: Pan Left shortcut.";
        }

        private void ButtonRebindPanLeft_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingPanLeft)
            {
                AcceptPanLeft();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingPanLeft = true;
                buttonRebindPanLeft.Text = "Cancel";
                buttonRebindPanLeft.Font = new Font(buttonRebindPanLeft.Font.FontFamily, buttonRebindPanLeft.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Pan Right

        private bool RebindingPanRight = false;
        private List<Keys> PanRightKeys = new List<Keys>();
        private List<System.Windows.Forms.PictureBox> PanRightPictures = new List<System.Windows.Forms.PictureBox>();

        private void UpdatePanRightKeyImages(List<Keys> key_list)
        {
            UpdateKeyImages(key_list, this.panelPanRight, PanRightPictures);
            if (RebindingPanRight)
            {
                if (key_list.Count == 0)
                {
                    buttonRebindPanRight.Text = "Cancel";
                }
                else
                {
                    buttonRebindPanRight.Text = "Accept";
                }
            }
        }

        void UpdatePanRightKeys()
        {
            FilterKeys(PanRightKeys);
            UpdatePanRightKeyImages(PanRightKeys);

            byte[] keys = new byte[PanRightKeys.Count];
            for (int i = 0; i < PanRightKeys.Count; ++i)
            {
                keys[i] = (byte)PanRightKeys[i];
            }
            Properties.Settings.Default.PanRightKeys = System.Convert.ToBase64String(keys);

            Program.SetShortcutKeys(6, keys);
        }

        private void AcceptPanRight()
        {
            this.TopMost = false;

            RebindingPanRight = false;
            buttonRebindPanRight.Text = "Rebind";
            buttonRebindPanRight.Font = new Font(buttonRebindPanRight.Font.FontFamily, buttonRebindPanRight.Font.Size, FontStyle.Regular);
            if (PressedKeys.Count > 0)
            {
                PanRightKeys = new List<Keys>(PressedKeys);
            }

            UpdatePanRightKeys();

            toolStripStatusLabel1.Text = "Rebound: Pan Right shortcut.";
        }

        private void ButtonRebindPanRight_Click(object sender, EventArgs e)
        {
            // If user pressed accept:
            if (RebindingPanRight)
            {
                AcceptPanRight();
            }
            else // If user pressed Rebind:
            {
                this.TopMost = true;

                RebindingPanRight = true;
                buttonRebindPanRight.Text = "Cancel";
                buttonRebindPanRight.Font = new Font(buttonRebindPanRight.Font.FontFamily, buttonRebindPanRight.Font.Size, FontStyle.Bold);
                PressedKeys.Clear();
            }

            this.KeyPreview = IsRebinding();
        }


        //----------------------------------------------------------------------
        // Keys

        private bool IsRebinding()
        {
            return RebindingRecenter || RebindingIncrease || RebindingDecrease || RebindingAltIncrease || RebindingAltDecrease || RebindingPanLeft || RebindingPanRight;
        }

        private List<Keys> PressedKeys = new List<Keys>();

        private bool HasKey(List<Keys> keys, Keys key)
        {
            return keys.Find(item => item == key) == key;
        }

        private void FilterKeys(List<Keys> keys)
        {
            if (HasKey(keys, Keys.ControlKey))
            {
                if (HasKey(keys, Keys.LControlKey) ||
                    HasKey(keys, Keys.RControlKey))
                {
                    keys.Remove(Keys.ControlKey);
                }
            }

            if (HasKey(keys, Keys.Shift))
            {
                if (HasKey(keys, Keys.LShiftKey) ||
                    HasKey(keys, Keys.RShiftKey))
                {
                    keys.Remove(Keys.ShiftKey);
                }
            }

            if (HasKey(keys, Keys.Menu))
            {
                if (HasKey(keys, Keys.LMenu) ||
                    HasKey(keys, Keys.RMenu))
                {
                    keys.Remove(Keys.Menu);
                }
            }
        }

        private Bitmap GetKeyBitmap(Keys keys)
        {
            switch (keys)
            {
            case Keys.LControlKey: return global::XRmonitorsUI.Properties.Resources.ctrl;
            case Keys.RControlKey: return global::XRmonitorsUI.Properties.Resources.ctrl_2;
            case Keys.LMenu: return global::XRmonitorsUI.Properties.Resources.alt;
            case Keys.RMenu: return global::XRmonitorsUI.Properties.Resources.alt_right;
            case Keys.LShiftKey: return global::XRmonitorsUI.Properties.Resources.shift;
            case Keys.RShiftKey: return global::XRmonitorsUI.Properties.Resources.shift_right;
            case Keys.D0: return global::XRmonitorsUI.Properties.Resources._0;
            case Keys.D1: return global::XRmonitorsUI.Properties.Resources._1;
            case Keys.D2: return global::XRmonitorsUI.Properties.Resources._2;
            case Keys.D3: return global::XRmonitorsUI.Properties.Resources._3;
            case Keys.D4: return global::XRmonitorsUI.Properties.Resources._4;
            case Keys.D5: return global::XRmonitorsUI.Properties.Resources._5;
            case Keys.D6: return global::XRmonitorsUI.Properties.Resources._6;
            case Keys.D7: return global::XRmonitorsUI.Properties.Resources._7;
            case Keys.D8: return global::XRmonitorsUI.Properties.Resources._8;
            case Keys.D9: return global::XRmonitorsUI.Properties.Resources._9;
            case Keys.A: return global::XRmonitorsUI.Properties.Resources.a;
            case Keys.B: return global::XRmonitorsUI.Properties.Resources.b;
            case Keys.C: return global::XRmonitorsUI.Properties.Resources.c;
            case Keys.D: return global::XRmonitorsUI.Properties.Resources.d;
            case Keys.E: return global::XRmonitorsUI.Properties.Resources.e;
            case Keys.F: return global::XRmonitorsUI.Properties.Resources.f;
            case Keys.G: return global::XRmonitorsUI.Properties.Resources.g;
            case Keys.H: return global::XRmonitorsUI.Properties.Resources.h;
            case Keys.I: return global::XRmonitorsUI.Properties.Resources.i;
            case Keys.J: return global::XRmonitorsUI.Properties.Resources.j;
            case Keys.K: return global::XRmonitorsUI.Properties.Resources.k;
            case Keys.L: return global::XRmonitorsUI.Properties.Resources.l;
            case Keys.M: return global::XRmonitorsUI.Properties.Resources.m;
            case Keys.N: return global::XRmonitorsUI.Properties.Resources.n;
            case Keys.O: return global::XRmonitorsUI.Properties.Resources.o;
            case Keys.P: return global::XRmonitorsUI.Properties.Resources.p;
            case Keys.Q: return global::XRmonitorsUI.Properties.Resources.q;
            case Keys.R: return global::XRmonitorsUI.Properties.Resources.r;
            case Keys.S: return global::XRmonitorsUI.Properties.Resources.s;
            case Keys.T: return global::XRmonitorsUI.Properties.Resources.t;
            case Keys.U: return global::XRmonitorsUI.Properties.Resources.u;
            case Keys.V: return global::XRmonitorsUI.Properties.Resources.v;
            case Keys.W: return global::XRmonitorsUI.Properties.Resources.w;
            case Keys.X: return global::XRmonitorsUI.Properties.Resources.x;
            case Keys.Y: return global::XRmonitorsUI.Properties.Resources.y;
            case Keys.Z: return global::XRmonitorsUI.Properties.Resources.z;
            case Keys.Oemtilde: return global::XRmonitorsUI.Properties.Resources.apostroph;
            case Keys.LWin: return global::XRmonitorsUI.Properties.Resources.love_key;
            case Keys.RWin: return global::XRmonitorsUI.Properties.Resources.love_key;
            case Keys.OemMinus: return global::XRmonitorsUI.Properties.Resources.minus;
            case Keys.Oemplus: return global::XRmonitorsUI.Properties.Resources.equals_plus;
            case Keys.Back: return global::XRmonitorsUI.Properties.Resources.backspace;
            case Keys.OemOpenBrackets: return global::XRmonitorsUI.Properties.Resources.bracket_open;
            case Keys.Oem6: return global::XRmonitorsUI.Properties.Resources.bracket_close;
            case Keys.Oem5: return global::XRmonitorsUI.Properties.Resources.backslash;
            case Keys.Oem1: return global::XRmonitorsUI.Properties.Resources.semicolon_dble;
            case Keys.Oem7: return global::XRmonitorsUI.Properties.Resources.comma;
            case Keys.Oemcomma: return global::XRmonitorsUI.Properties.Resources.comma_lt;
            case Keys.OemPeriod: return global::XRmonitorsUI.Properties.Resources.period_gt;
            case Keys.OemQuestion: return global::XRmonitorsUI.Properties.Resources.slash_questionmark;
            case Keys.NumLock: return global::XRmonitorsUI.Properties.Resources.num_lock;
            case Keys.Tab: return global::XRmonitorsUI.Properties.Resources.tab;
            case Keys.Capital: return global::XRmonitorsUI.Properties.Resources.capslock;
            case Keys.Return: return global::XRmonitorsUI.Properties.Resources.enter;
            case Keys.Apps: return global::XRmonitorsUI.Properties.Resources.apps;
            case Keys.Insert: return global::XRmonitorsUI.Properties.Resources.insert;
            case Keys.Home: return global::XRmonitorsUI.Properties.Resources.home;
            case Keys.PageUp: return global::XRmonitorsUI.Properties.Resources.page_up;
            case Keys.Next: return global::XRmonitorsUI.Properties.Resources.page_down;
            case Keys.End: return global::XRmonitorsUI.Properties.Resources.end;
            case Keys.Delete: return global::XRmonitorsUI.Properties.Resources.delete;
            case Keys.Left: return global::XRmonitorsUI.Properties.Resources.cursor_left;
            case Keys.Right: return global::XRmonitorsUI.Properties.Resources.cursor_right;
            case Keys.Up: return global::XRmonitorsUI.Properties.Resources.cursor_up;
            case Keys.Down: return global::XRmonitorsUI.Properties.Resources.cursor_down;
            case Keys.Escape: return global::XRmonitorsUI.Properties.Resources.esc;
            case Keys.F1: return global::XRmonitorsUI.Properties.Resources.f1;
            case Keys.F2: return global::XRmonitorsUI.Properties.Resources.f2;
            case Keys.F3: return global::XRmonitorsUI.Properties.Resources.f3;
            case Keys.F4: return global::XRmonitorsUI.Properties.Resources.f4;
            case Keys.F5: return global::XRmonitorsUI.Properties.Resources.f5;
            case Keys.F6: return global::XRmonitorsUI.Properties.Resources.f6;
            case Keys.F7: return global::XRmonitorsUI.Properties.Resources.f7;
            case Keys.F8: return global::XRmonitorsUI.Properties.Resources.f8;
            case Keys.F9: return global::XRmonitorsUI.Properties.Resources.f9;
            case Keys.F10: return global::XRmonitorsUI.Properties.Resources.f10;
            case Keys.F11: return global::XRmonitorsUI.Properties.Resources.f11;
            case Keys.F12: return global::XRmonitorsUI.Properties.Resources.f12;
            case Keys.Scroll: return global::XRmonitorsUI.Properties.Resources.scroll_lock;
            case Keys.Pause: return global::XRmonitorsUI.Properties.Resources.pause;
            case Keys.Divide: return global::XRmonitorsUI.Properties.Resources.divide;
            case Keys.Multiply: return global::XRmonitorsUI.Properties.Resources.multiply;
            case Keys.NumPad7: return global::XRmonitorsUI.Properties.Resources.NumPad7;
            case Keys.NumPad8: return global::XRmonitorsUI.Properties.Resources.NumPad8;
            case Keys.NumPad9: return global::XRmonitorsUI.Properties.Resources.NumPad9;
            case Keys.Add: return global::XRmonitorsUI.Properties.Resources.add;
            case Keys.Subtract: return global::XRmonitorsUI.Properties.Resources.subtract;
            case Keys.NumPad4: return global::XRmonitorsUI.Properties.Resources.NumPad4;
            case Keys.NumPad5: return global::XRmonitorsUI.Properties.Resources.NumPad5;
            case Keys.NumPad6: return global::XRmonitorsUI.Properties.Resources.NumPad6;
            case Keys.NumPad1: return global::XRmonitorsUI.Properties.Resources.NumPad1;
            case Keys.NumPad2: return global::XRmonitorsUI.Properties.Resources.NumPad2;
            case Keys.NumPad3: return global::XRmonitorsUI.Properties.Resources.NumPad3;
            case Keys.NumPad0: return global::XRmonitorsUI.Properties.Resources.NumPad0;
            case Keys.Decimal: return global::XRmonitorsUI.Properties.Resources.Decimal;
            case Keys.Space: return global::XRmonitorsUI.Properties.Resources.spacebar;
            }

            return null;
        }

        private void UpdateKeyImages(List<Keys> key_list, Panel panel, List<PictureBox> picture_list)
        {
            foreach (var picture in picture_list)
            {
                panel.Controls.Remove(picture);
            }
            picture_list.Clear();

            int offset_x = 0;
            bool previous = false;

            foreach (var key in key_list)
            {
                //Console.WriteLine("Pressed: " + key.ToString());

                var bitmap = GetKeyBitmap(key);
                if (bitmap == null)
                {
                    Console.WriteLine("Missing bitmap for key!");
                    continue;
                }

                if (previous)
                {
                    float plus_height = panelRecenter.Height * 0.5f;
                    float plus_offset_y = plus_height * 0.4f;

                    var plus = new System.Windows.Forms.PictureBox();
                    plus.BackgroundImage = global::XRmonitorsUI.Properties.Resources.plus;
                    plus.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
                    plus.Location = new System.Drawing.Point(offset_x, (int)plus_offset_y);
                    plus.Margin = new System.Windows.Forms.Padding(5);
                    plus.Size = new System.Drawing.Size((int)plus_height, (int)plus_height);
                    plus.MaximumSize = new System.Drawing.Size((int)plus_height, (int)plus_height);
                    plus.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
                    plus.TabStop = false;

                    picture_list.Add(plus);
                    panel.Controls.Add(plus);

                    offset_x += (int)plus_height;
                }

                float height = panelRecenter.Height;
                float aspect = bitmap.Width / (float)bitmap.Height;
                float new_width = aspect * height;
                float offset_y = 0.0f;

                var box = new System.Windows.Forms.PictureBox();
                box.BackgroundImage = bitmap;
                box.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
                box.Location = new System.Drawing.Point(offset_x, (int)offset_y);
                box.Margin = new System.Windows.Forms.Padding(5);
                box.Size = new System.Drawing.Size((int)new_width, (int)height);
                box.MaximumSize = new System.Drawing.Size((int)new_width, (int)height);
                box.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
                box.TabStop = false;

                picture_list.Add(box);
                panel.Controls.Add(box);

                previous = true;

                offset_x += (int)new_width;
            }
        }

        void AddPressedKey(Keys key)
        {
            PressedKeys.Remove(key);
            PressedKeys.Add(key);
        }

        void RemovePressedKey(Keys key)
        {
            PressedKeys.Remove(key);
        }

        private void UpdateModifiers()
        {
            if (Keyboard.IsKeyDown(Key.LeftAlt))
            {
                AddPressedKey(Keys.LMenu);
            }
            else
            {
                RemovePressedKey(Keys.LMenu);
            }

            if (Keyboard.IsKeyDown(Key.LeftCtrl))
            {
                AddPressedKey(Keys.LControlKey);
            }
            else
            {
                RemovePressedKey(Keys.LControlKey);
            }

            if (Keyboard.IsKeyDown(Key.LeftShift))
            {
                AddPressedKey(Keys.LShiftKey);
            }
            else
            {
                RemovePressedKey(Keys.LShiftKey);
            }

            if (Keyboard.IsKeyDown(Key.RightAlt))
            {
                AddPressedKey(Keys.RMenu);
            }
            else
            {
                RemovePressedKey(Keys.RMenu);
            }

            if (Keyboard.IsKeyDown(Key.RightCtrl))
            {
                AddPressedKey(Keys.RControlKey);
            }
            else
            {
                RemovePressedKey(Keys.RControlKey);
            }

            if (Keyboard.IsKeyDown(Key.RightShift))
            {
                AddPressedKey(Keys.RShiftKey);
            }
            else
            {
                RemovePressedKey(Keys.RShiftKey);
            }
        }

        protected override bool ProcessDialogKey(Keys keyData)
        {
            if (IsRebinding())
            {
                return false;
            }

            return base.ProcessDialogKey(keyData);
        }

        List<Keys> LastKeyUpdate = new List<Keys>();

        private void HandleKeyChange(KeyEventArgs e)
        {
            if (!IsRebinding())
            {
                return;
            }

            e.Handled = true;
            e.SuppressKeyPress = true;

            if (Enumerable.SequenceEqual(LastKeyUpdate.OrderBy(t => t), PressedKeys.OrderBy(t => t)))
            {
                return;
            }
            LastKeyUpdate = new List<Keys>(PressedKeys);

            if (RebindingRecenter)
            {
                UpdateRecenterKeyImages(PressedKeys);
            }
            if (RebindingIncrease)
            {
                UpdateIncreaseKeyImages(PressedKeys);
            }
            if (RebindingDecrease)
            {
                UpdateDecreaseKeyImages(PressedKeys);
            }
            if (RebindingAltIncrease)
            {
                UpdateAltIncreaseKeyImages(PressedKeys);
            }
            if (RebindingAltDecrease)
            {
                UpdateAltDecreaseKeyImages(PressedKeys);
            }
            if (RebindingPanLeft)
            {
                UpdatePanLeftKeyImages(PressedKeys);
            }
            if (RebindingPanRight)
            {
                UpdatePanRightKeyImages(PressedKeys);
            }

            // Stop rebinding once a non-modifier key is pressed
            foreach (Keys key in PressedKeys)
            {
                // If this is a modifier key:
                if (key == Keys.Alt || key == Keys.Menu || key == Keys.LMenu || key == Keys.RMenu ||
                    key == Keys.Control || key == Keys.ControlKey || key == Keys.LControlKey || key == Keys.RControlKey ||
                    key == Keys.Shift || key == Keys.ShiftKey || key == Keys.LShiftKey || key == Keys.RShiftKey ||
                    key == Keys.LWin || key == Keys.RWin ||
                    key == Keys.CapsLock)
                {
                    // Keep looking
                }
                else
                {
                    if (RebindingRecenter)
                    {
                        AcceptRecenter();
                    }
                    if (RebindingIncrease)
                    {
                        AcceptIncrease();
                    }
                    if (RebindingDecrease)
                    {
                        AcceptDecrease();
                    }
                    if (RebindingAltIncrease)
                    {
                        AcceptAltIncrease();
                    }
                    if (RebindingAltDecrease)
                    {
                        AcceptAltDecrease();
                    }
                    if (RebindingPanLeft)
                    {
                        AcceptPanLeft();
                    }
                    if (RebindingPanRight)
                    {
                        AcceptPanRight();
                    }
                    this.KeyPreview = false;
                    break;
                }
            }
        }

        private void MainForm_KeyDown(object sender, KeyEventArgs e)
        {
            UpdateModifiers();
            AddPressedKey(e.KeyCode);
            HandleKeyChange(e);
            if (IsRebinding())
            {
                e.SuppressKeyPress = true;
            }
        }

        private void MainForm_KeyUp(object sender, KeyEventArgs e)
        {
            UpdateModifiers();
            RemovePressedKey(e.KeyCode);
            HandleKeyChange(e);
            if (IsRebinding())
            {
                e.SuppressKeyPress = true;
            }
        }

        private void MainForm_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (IsRebinding())
            {
                e.Handled = true;
            }
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            timer1.Enabled = false;

            Properties.Settings.Default.Save();

            StopVrProcess();

            Application.Exit();
        }

        private void ShowVirtualMonitorsSettingsToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            groupVMSettings.Visible = showVMSettings.Checked;
            Properties.Settings.Default.ShowVMSettings = showVMSettings.Checked;
            //UpdateFormMinimumSize();
        }

        private void ShowWindowsMixedRealityToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {
            groupWMR.Visible = showWMR.Checked;
            Properties.Settings.Default.ShowWMR = showWMR.Checked;
            //UpdateFormMinimumSize();
        }

        private void UpdateFormMinimumSize()
        {
            int width = 0;
            int height = 0;

            if (groupVMSettings.Visible)
            {
                if (width < groupVMSettings.Width)
                {
                    width = groupVMSettings.Width;
                }
                height += groupVMSettings.Height;
            }

            if (groupWMR.Visible)
            {
                if (width < groupWMR.Width)
                {
                    width = groupWMR.Width;
                }
                height += groupWMR.Height;
            }
            height += groupVMGo.Height;

            width += 32;
            height += 80;

            this.MinimumSize = new System.Drawing.Size(width, height);
            this.Size = new System.Drawing.Size(width, height);
        }

        private AboutBox m_AboutBox = new AboutBox();
        private ReportIssue m_ReportIssue = new ReportIssue();

        private void AboutToolStripMenuItem_Click_1(object sender, EventArgs e)
        {
            m_AboutBox.ShowDialog();
        }

        private void ReportIssueToolStripMenuItem_Click_1(object sender, EventArgs e)
        {
            m_ReportIssue.ShowDialog();
        }

        private void DisableWinY_CheckedChanged(object sender, EventArgs e)
        {
            Program.SetDisableWinY(disableWinY.Checked);
            Properties.Settings.Default.DisableWinY = disableWinY.Checked;

            if (disableWinY.Checked)
            {
                toolStripStatusLabel1.Text = "Enabled: Automatic WinKey + Y keypress when entering VR.";
            }
            else
            {
                toolStripStatusLabel1.Text = "Disabled: Manual WinKey + Y keypress will be required.";
            }
        }

        private void Timer1_Tick(object sender, EventArgs e)
        {
            // If Vr app should be stopped:
            if (!VrStarted)
            {
                if (VrProc != null)
                {
                    Console.WriteLine("Force-killing unresponsive child");
                    try
                    {
                        VrProc.Kill();
                    }
                    catch (Exception)
                    {
                    }
                }
            }
        }

        private void CheckAutoStart_CheckedChanged_1(object sender, EventArgs e)
        {
            try
            {
                if (checkAutoStart.Checked)
                {
                    Console.WriteLine("Adding startup regkey");
                    string path = FindExePath();
                    Console.WriteLine("Found exe path: " + path);
                    AddStartup("XRmonitors", path);
                    toolStripStatusLabel1.Text = "Enabled: XRmonitors will launch when you log in.";
                }
                else
                {
                    Console.WriteLine("Removing startup regkey");
                    RemoveStartup("XRmonitors");
                    toolStripStatusLabel1.Text = "Disabled: XRmonitors will not launch when you log in.";
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Failed to set up AutoStart: ", ex.ToString());
                checkAutoStart.Enabled = false;
            }
        }

        private void CheckBlue_CheckedChanged(object sender, EventArgs e)
        {
            Program.SetBlueLightFilter(checkBlue.Checked);
            Properties.Settings.Default.BlueLightFilter = checkBlue.Checked;

            if (checkBlue.Checked)
            {
                toolStripStatusLabel1.Text = "Enabled: VR headset will reduce monitor blue light.";
            }
            else
            {
                toolStripStatusLabel1.Text = "Disabled: Normal color will be displayed.";
            }
        }

        private void MainForm_Shown(object sender, EventArgs e)
        {
            this.Activate();
            this.TopMost = false;
        }
    }
}
