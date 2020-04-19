using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using IWshRuntimeLibrary;

namespace XRmonitorsInstaller
{
    static class Program
    {
        public static bool Installing = false;
        public static bool Uninstalling = false;
        public static bool Upgrading = false;
        public static bool IsInstalled = false;
        public static bool Register = false;
        public static string RegisterSerial = "";

        public static SetupForm setup_form = null;

        public static Assembly i10 = null;

        public static Guid XRmonitorsGuid = new Guid();

        public static string InstallPath = "";

        public static string[] FileNames = new string[] {
            "XRmonitorsHologram.exe",
            "XRmonitorsCamera.dll",
            "XRmonitorsService.exe",
            "XRmonitorsUI.exe",
            "ToggleSwitch.dll",
            "Microsoft.VisualStudio.Utilities.dll",
            "Newtonsoft.Json.dll",
            "camera_calibration.ini",
            //"XRmonitorsInstaller.exe",
            "XRmonitorsSetup.exe"
        };

        public static string getTempResourcePath(string name)
        {
            string tempPath = Path.GetTempPath();
            string libraryFile = Path.Combine(tempPath, "XRmonitors", name);
            return libraryFile;
        }


        public static bool InstallService(bool report_failures = true)
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsService.exe");

            try
            {
                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = exe_path;
                psi.Arguments = "/silent /install";

                Process process = Process.Start(psi);
                process.WaitForExit();
                if (process.ExitCode == 0)
                {
                    return true;
                }
            }
            catch (Exception e)
            {
                if (report_failures)
                {
                    MessageBox.Show("Installing the Windows service failed.  Error: " + e.ToString() + "  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Installation Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            if (report_failures)
            {
                MessageBox.Show("Installing the Windows service failed.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Installation Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }

            return false;
        }

        public static bool StartService(bool report_failures = true)
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsService.exe");

            try
            {
                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = exe_path;
                psi.Arguments = "/silent /start";

                Process process = Process.Start(psi);
                process.WaitForExit();
                if (process.ExitCode == 0)
                {
                    return true;
                }
            }
            catch (Exception e)
            {
                if (report_failures)
                {
                    MessageBox.Show("Starting the Windows service failed.  Error: " + e.ToString() + "  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Installation Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            if (report_failures)
            {
                MessageBox.Show("Starting the Windows service failed.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Installation Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }

            return false;
        }

        public static bool StopService(bool report_failures = false)
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsService.exe");

            try
            {
                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = exe_path;
                psi.Arguments = "/silent /stop";

                Console.WriteLine("Attempting to stop service");

                Process process = Process.Start(psi);
                process.WaitForExit();

                Console.WriteLine("Service stop command exited with code: {0}", process.ExitCode);

                if (process.ExitCode == 0)
                {
                    return true;
                }
            }
            catch (Exception e)
            {
                if (report_failures)
                {
                    MessageBox.Show("Stopping the Windows service failed.  Error: " + e.ToString() + "  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstall Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            if (report_failures)
            {
                MessageBox.Show("Stopping the Windows service failed.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstall Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }

            return false;
        }

        public static bool UninstallService(bool report_failures = false)
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsService.exe");

            try
            {
                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = exe_path;
                psi.Arguments = "/silent /uninstall";

                Console.WriteLine("Attempting to uninstall service");

                Process process = Process.Start(psi);
                process.WaitForExit();

                Console.WriteLine("Service uninstall command exited with code: {0}", process.ExitCode);

                if (process.ExitCode == 0)
                {
                    return true;
                }
            }
            catch (Exception e)
            {
                if (report_failures)
                {
                    MessageBox.Show("Uninstalling the Windows service failed.  Error: " + e.ToString() + "  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstall Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            if (report_failures)
            {
                MessageBox.Show("Uninstalling the Windows service failed.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstall Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
            }

            return false;
        }

        public static bool WriteInstallPath(string path)
        {
            string XrMonitorsRegKeyPath = @"SOFTWARE";

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(XrMonitorsRegKeyPath, true))
            {
                if (parent == null)
                {
                    return false;
                }

                try
                {
                    RegistryKey key = null;

                    key = parent.OpenSubKey("XRmonitors", true) ??
                        parent.CreateSubKey("XRmonitors");

                    if (key == null)
                    {
                        return false;
                    }

                    try
                    {
                        string guidText = Program.XRmonitorsGuid.ToString("B");
                        key.SetValue(guidText, path);

                        return true;
                    }
                    finally
                    {
                        if (key != null)
                        {
                            key.Close();
                        }
                    }
                }
                catch (Exception)
                {
                }
            }

            return false;
        }

        public static string ReadInstallPath()
        {
            string XrMonitorsRegKeyPath = @"SOFTWARE\XRmonitors";

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(XrMonitorsRegKeyPath, false))
            {
                if (parent == null)
                {
                    return "";
                }

                try
                {
                    RegistryKey key = null;

                    try
                    {
                        string guidText = Program.XRmonitorsGuid.ToString("B");
                        object obj = parent.GetValue(guidText);

                        if (obj == null)
                        {
                            return "";
                        }

                        string str = obj as string;
                        return str;
                    }
                    finally
                    {
                        if (key != null)
                        {
                            key.Close();
                        }
                    }
                }
                catch (Exception)
                {
                }
            }

            return "";
        }

        public static bool RemoveInstallPath()
        {
            string XrMonitorsRegKeyPath = @"SOFTWARE\XRmonitors";

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(XrMonitorsRegKeyPath, true))
            {
                if (parent == null)
                {
                    MessageBox.Show("Failed to open registry key for uninstall(1).  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
                try
                {
                    string guidText = Program.XRmonitorsGuid.ToString("B");
                    parent.DeleteValue(guidText);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("An error occurred removing install path from the registry: " + ex.ToString() + "  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            return true;
        }

        public static bool RemoveUninstaller()
        {
            string UninstallRegKeyPath = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall";

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(UninstallRegKeyPath, true))
            {
                if (parent == null)
                {
                    MessageBox.Show("Failed to open registry key for uninstall.  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
                try
                {
                    string guidText = Program.XRmonitorsGuid.ToString("B");
                    parent.DeleteSubKeyTree(guidText);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("An error occurred removing uninstall information from the registry: " + ex.ToString() + "  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            return true;
        }

        public static bool CreateUninstaller()
        {
            string install_path = Program.InstallPath;
            string exe_path = Path.Combine(install_path, "XRmonitorsSetup.exe");

            string UninstallRegKeyPath = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall";

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(UninstallRegKeyPath, true))
            {
                if (parent == null)
                {
                    MessageBox.Show("Failed to open registry key for uninstall.  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Install Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
                try
                {
                    RegistryKey key = null;

                    try
                    {
                        string guidText = Program.XRmonitorsGuid.ToString("B");
                        key = parent.OpenSubKey(guidText, true) ??
                              parent.CreateSubKey(guidText);

                        if (key == null)
                        {
                            MessageBox.Show("Failed to create registry key for uninstall.  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Install Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                            return false;
                        }

                        Version version = Assembly.GetEntryAssembly().GetName().Version;

                        key.SetValue("DisplayName", "XRmonitors");
                        key.SetValue("ApplicationVersion", version.ToString());
                        key.SetValue("Publisher", "Augmented Perception Corporation");
                        key.SetValue("DisplayIcon", exe_path);
                        key.SetValue("DisplayVersion", version.ToString(3));
                        key.SetValue("URLInfoAbout", "https://xrmonitors.com");
                        key.SetValue("Contact", "support@xrmonitors.com");
                        key.SetValue("InstallDate", DateTime.Now.ToString("yyyyMMdd"));
                        key.SetValue("UninstallString", exe_path + " /uninstall");
                        key.SetValue("Comments", "XRmonitors - Virtual multi-monitors for the Workplace");
                    }
                    finally
                    {
                        if (key != null)
                        {
                            key.Close();
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show("An error occurred writing uninstall information to the registry: " + ex.ToString() + "  \n\nThe service is fully installed but can only be uninstalled manually through the command line.  \n\nPlease contact support at support@xrmonitors.com.", "XRmonitors - Uninstaller Install Failure", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    return false;
                }
            }

            return true;
        }

        public static void AddShortcuts()
        {
            try
            {
                string pathToExe = Path.Combine(Program.InstallPath, "XRmonitorsUI.exe");
                string commonStartMenuPath = Environment.GetFolderPath(Environment.SpecialFolder.CommonStartMenu);
                string appStartMenuPath = Path.Combine(commonStartMenuPath, "Programs", "XRmonitors");

                if (!Directory.Exists(appStartMenuPath))
                {
                    Directory.CreateDirectory(appStartMenuPath);
                }

                string shortcutLocation = Path.Combine(appStartMenuPath, "XRmonitors" + ".lnk");

                WshShell shell = new WshShell();
                IWshShortcut shortcut = (IWshShortcut)shell.CreateShortcut(shortcutLocation);

                shortcut.Description = "XRmonitors Virtual Multi-Monitors for the Workplace";
                shortcut.IconLocation = pathToExe + ",0";
                shortcut.TargetPath = pathToExe;
                shortcut.Save();

                shortcut = null;
                shell = null;


                string commonDesktopPath = Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory);

                string desktopShortcutLocation = Path.Combine(commonDesktopPath, "XRmonitors" + ".lnk");

                shell = new WshShell();
                shortcut = (IWshShortcut)shell.CreateShortcut(desktopShortcutLocation);

                shortcut.Description = "XRmonitors Virtual Multi-Monitors for the Workplace";
                shortcut.IconLocation = pathToExe + ",0";
                shortcut.TargetPath = pathToExe;
                shortcut.Save();
            }
            catch (Exception e)
            {
                MessageBox.Show("Error while adding shortcuts to the start menu: " + e.ToString(), "XRmonitors - Adding Shortcuts Failed");
            }
        }

        public static void RemoveShortcuts()
        {
            try
            {
                string pathToExe = Path.Combine(Program.InstallPath, "XRmonitorsUI.exe");
                string commonStartMenuPath = Environment.GetFolderPath(Environment.SpecialFolder.CommonStartMenu);
                string appStartMenuPath = Path.Combine(commonStartMenuPath, "Programs", "XRmonitors");

                if (Directory.Exists(appStartMenuPath))
                {
                    Directory.Delete(appStartMenuPath, true);
                }

                string commonDesktopPath = Environment.GetFolderPath(Environment.SpecialFolder.CommonDesktopDirectory);

                string desktopShortcutLocation = Path.Combine(commonDesktopPath, "XRmonitors" + ".lnk");

                if (System.IO.File.Exists(desktopShortcutLocation))
                {
                    System.IO.File.Delete(desktopShortcutLocation);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Error while removing shortcuts from start menu: " + e.ToString(), "XRmonitors - Remove Shortcuts Failed");
            }
        }

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


        //----------------------------------------------------------------------
        // Logger
        //----------------------------------------------------------------------

        private static FileStream LoggerStream = null;
        private static StreamWriter LoggerWriter = null;

        static void OnProcessExit(object sender, EventArgs e)
        {
            Console.WriteLine("Process exited normally");
            LoggerWriter.Flush();
        }

        static public void StartLoggingToFile(string file_prefix)
        {
            TextWriter oldOut = Console.Out;
            try
            {
                string program_data_path = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData);
                string file_name = file_prefix + "." + DateTime.Now.ToString("MM_dd_yyyy_h_mm") + ".log";
                string install_log_file = Path.Combine(program_data_path, "XRmonitors", file_name);

                LoggerStream = new FileStream(install_log_file, FileMode.OpenOrCreate, FileAccess.Write);
                LoggerWriter = new StreamWriter(LoggerStream);

                AppDomain.CurrentDomain.ProcessExit += new EventHandler(OnProcessExit);
            }
            catch (Exception e)
            {
                Console.WriteLine("Cannot open log file for writing");
                Console.WriteLine(e.Message);
                return;
            }
            Console.SetOut(LoggerWriter);
            Console.WriteLine("Installer log opened: " + DateTime.Now.ToString("MM/dd/yyyy h:mm tt"));
        }


        //----------------------------------------------------------------------
        // Entrypoint
        //----------------------------------------------------------------------

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            StartLoggingToFile("XRmonitorsInstaller");

            XRmonitorsGuid = new Guid("1513D568-C504-4B24-A49D-2F532AEEB824");

            bool result;
            var mutex = new System.Threading.Mutex(true, "XRmonitorsInstallerUnique", out result);
            if (!result)
            {
                MessageBox.Show("Another instance of XRmonitorsInstaller.exe is already running.  \n\nClose the other instance of the program before running.", "XRmonitors - Installer");
                return;
            }

            InstallPath = Program.ReadInstallPath();
            IsInstalled = Program.InstallPath.Length > 0;

            for (int i = 0; i < args.Length; ++i)
            {
                string arg = args[i];

                if (0 == arg.CompareTo("/uninstall"))
                {
                    Console.WriteLine("Uninstalling");
                    Uninstalling = true;
                    break;
                }
                if (0 == arg.CompareTo("/install"))
                {
                    Console.WriteLine("Installing");
                    Installing = true;
                    break;
                }
                if (0 == arg.CompareTo("/upgrade"))
                {
                    Console.WriteLine("Upgrading");
                    Upgrading = true;
                    break;
                }
                if (0 == arg.CompareTo("/register"))
                {
                    if (i + i < args.Length)
                    {
                        RegisterSerial = args[i + 1];
                    }

                    Console.WriteLine("Register: {0}", RegisterSerial);
                    Register = true;
                    break;
                }
            }

            // Default to installing
            if (!Uninstalling && !Upgrading && !Register)
            {
                Console.WriteLine("Installing(default)");
                Installing = true;
            }

            setup_form = new SetupForm();

            Application.Run(setup_form);
        }
    }
}
