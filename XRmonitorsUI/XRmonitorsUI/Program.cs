using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsUI
{
    class Program
    {
        private static XRmonitorsUI.MainForm g_MainForm = null;
        public static XRmonitorsUI.OpenXrNeeded g_OpenXrNeeded = null;

        public static Guid XRmonitorsGuid = new Guid();

        public static string InstallPath = "";

        private static UInt32 UiEpoch = 0;

        public static MemoryMappedFile MappedFile = null;
        public static MemoryMappedViewAccessor MappedAccessor = null;

        public static int GetReleaseId()
        {
            string CurrentVersionPath = @"SOFTWARE\Microsoft\Windows NT\CurrentVersion";
            int value = 0;

            using (RegistryKey parent = Registry.LocalMachine.OpenSubKey(CurrentVersionPath, false))
            {
                if (parent == null)
                {
                    Console.WriteLine("Failed to open CurrentVersion regkey");
                    return 0;
                }

                try
                {
                    RegistryKey key = null;

                    try
                    {
                        object obj = parent.GetValue("ReleaseId");

                        if (obj == null)
                        {
                            Console.WriteLine("Failed to get ReleaseId regkey");
                        }
                        else
                        {
                            string release_id_str = obj as string;
                            if (release_id_str == null || !int.TryParse(release_id_str, out value))
                            {
                                Console.WriteLine("Failed to convert ReleaseId to integer");
                            }
                        }
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
                    Console.WriteLine("Exception while reading ReleaseId: " + ex.Message);
                }
            }

            return value;
        }

        public static bool IsProgramInstalled(string programDisplayName)
        {
            Console.WriteLine(string.Format("Checking install status of: {0}", programDisplayName));
            foreach (var item in Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall").GetSubKeyNames())
            {
                object programName = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + item).GetValue("DisplayName");

                Console.WriteLine(programName);

                if (string.Equals(programName, programDisplayName))
                {
                    Console.WriteLine("Install status: INSTALLED");
                    return true;
                }
            }
            Console.WriteLine("Install status: NOT INSTALLED");
            return false;
        }

        public static bool IsStoreAppInstalled(string programDisplayName)
        {
            Console.WriteLine(string.Format("Checking install status of app: {0}", programDisplayName));

            foreach (string item in Registry.ClassesRoot.OpenSubKey("ActivatableClasses\\Package").GetSubKeyNames())
            {
                if (item.StartsWith(programDisplayName))
                {
                    Console.WriteLine("Install status: INSTALLED");
                    return true;
                }
            }
            Console.WriteLine("Install status: NOT INSTALLED");
            return false;
        }

        public static void SetParentPid()
        {
            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    UInt32 pid = (UInt32)Process.GetCurrentProcess().Id;
                    MappedAccessor.Write<UInt32>(64, ref pid);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void SetDisableWinY(bool disable)
        {
            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    byte structure = 0;
                    if (disable)
                    {
                        structure = 1;
                    }
                    MappedAccessor.Write<byte>(64 + 4, ref structure);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void SetEnablePassthrough(bool enable)
        {
            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    byte structure = 0;
                    if (enable)
                    {
                        structure = 1;
                    }
                    MappedAccessor.Write<byte>(64 + 5, ref structure);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void SetBlueLightFilter(bool enable)
        {
            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    byte structure = 0;
                    if (enable)
                    {
                        structure = 1;
                    }
                    MappedAccessor.Write<byte>(64 + 6, ref structure);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void SetEnableTerminated(bool enable)
        {
            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    byte structure = 0;
                    if (enable)
                    {
                        structure = 1;
                    }
                    MappedAccessor.Write<byte>(64 + 7, ref structure);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void SetShortcutKeys(int number, byte[] keys)
        {
            byte[] padded = new byte[8];
            for (int i = 0; i < 8 && i < keys.Length; ++i)
            {
                padded[i] = keys[i];
            }

            try
            {
                if (MappedAccessor != null)
                {
                    ++UiEpoch;

                    UInt32 epoch = UiEpoch;
                    MappedAccessor.Write<UInt32>(0, ref epoch);

                    MappedAccessor.WriteArray<byte>(64 + 8 + 8 * number, padded, 0, 8);

                    UInt32 epochAfter = UiEpoch;
                    MappedAccessor.Write<UInt32>(32, ref epochAfter);
                }
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to write shared memory region: " + e.ToString(), "XRmonitors - Configuration Failed");
            }
        }

        public static void StartMemoryMappedFile()
        {
            try
            {
                string mapped_file_name = "Global\\XRmonitorsUI";
                MappedFile = MemoryMappedFile.OpenExisting(mapped_file_name);
                MappedAccessor = MappedFile.CreateViewAccessor();

                SetParentPid();
            }
            catch (Exception e)
            {
                MessageBox.Show("Shared memory region could not be set up: " + e.ToString(), "XRmonitors - Configuration Failed");
                MappedAccessor = null;
            }
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
                        string guidText = XRmonitorsGuid.ToString("B");
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
                string program_data_path = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
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

        static public void ShowMainForm()
        {
            if (g_MainForm == null)
            {
                g_MainForm = new XRmonitorsUI.MainForm();
            }
            Program.g_MainForm.Show();
        }

        static private System.Threading.Mutex g_Mutex = null;

        [STAThread]
        static void Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            bool createdNew, success = false;
            g_Mutex = new System.Threading.Mutex(true, "XRmonitorsUIUnique", out createdNew);
            if (g_Mutex != null)
            {
                success = true;
            }

            if (!success || !createdNew)
            {
                MessageBox.Show("Another instance of XRmonitorsUI.exe is already running.  \n\nClose the other instance of the program before running.", "XRmonitors - Already Running");
                return;
            }

            StartLoggingToFile("XRmonitorsUI");

            XRmonitorsGuid = new Guid("1513D568-C504-4B24-A49D-2F532AEEB824");
            InstallPath = ReadInstallPath();

            StartMemoryMappedFile();

            // Create these after setup so that the dependencies above are available
            g_OpenXrNeeded = new XRmonitorsUI.OpenXrNeeded();

            g_MainForm = new XRmonitorsUI.MainForm();
            Application.Run(Program.g_MainForm);
        }
    }
}
