using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace XRmonitorsSetup
{
    class Program
    {
        public static bool Installing = false;
        public static bool Uninstalling = false;
        public static bool Upgrading = false;
        public static bool Register = false;
        public static string RegisterSerial = "";

        public static string getTempResourcePath(string name)
        {
            string tempPath = Path.GetTempPath();
            string libraryFile = Path.Combine(tempPath, "XRmonitors", name);
            return libraryFile;
        }
        public static bool copyResourceFile(string name)
        {
            try
            {
                Assembly assembly = Assembly.GetExecutingAssembly();
                string assemblyName = assembly.FullName.Split(',')[0];
                string resourceName = assemblyName + ".Resources." + name;

                Stream imageStream = assembly.GetManifestResourceStream(resourceName);
                long bytestreamMaxLength = imageStream.Length;
                byte[] buffer = new byte[bytestreamMaxLength];
                imageStream.Read(buffer, 0, (int)bytestreamMaxLength);

                string libraryFile = getTempResourcePath(name);

                BinaryWriter writer = new BinaryWriter(File.Open(libraryFile, FileMode.Create));
                writer.Write(buffer);
                writer.Close();

                Console.WriteLine("Extracted: " + libraryFile);
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to extract embedded file: " + name + "  \n\nError: " + e.ToString());
                return false;
            }

            return true;
        }

        public static string[] FileNames = new string[] {
            "ToggleSwitch.dll",
            "camera_calibration.ini",
            "XRmonitorsHologram.exe",
            "XRmonitorsCamera.dll",
            "Microsoft.VisualStudio.Utilities.dll",
            "Newtonsoft.Json.dll",
            "XRmonitorsService.exe",
            "XRmonitorsUI.exe",
            "XRmonitorsInstaller.exe"
        };

        public static bool extractDependencies()
        {
            Console.WriteLine("Creating temporary folder for extraction");

            try
            {
                string tempPath = Path.GetTempPath();
                string dirPath = Path.Combine(tempPath, "XRmonitors");
                Directory.CreateDirectory(dirPath);

                Console.WriteLine("Created directory: " + dirPath);
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to create temporary directory for extracting embedded files.  \n\nError: " + e.ToString());
                return false;
            }

            Console.WriteLine("Extract dependencies");

            if (Program.Uninstalling)
            {
                if (!copyResourceFile("XRmonitorsInstaller.exe"))
                {
                    return false;
                }
            }
            else
            {
                foreach (var name in FileNames)
                {
                    if (!copyResourceFile(name))
                    {
                        return false;
                    }
                }
            }

            try
            {
                string temp_path = getTempResourcePath("XRmonitorsSetup.exe");

                if (File.Exists(temp_path))
                {
                    File.Delete(temp_path);
                }
                File.Copy(Assembly.GetExecutingAssembly().Location, temp_path);
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to copy setup program to temporary directory.  \n\nError: " + e.ToString());
                return false;
            }

            return true;
        }

        public static void deleteDependencies()
        {
            Console.WriteLine("Delete dependencies");

            foreach (var name in FileNames)
            {
                try
                {
                    File.Delete(getTempResourcePath(name));
                }
                catch (Exception e)
                {
                    MessageBox.Show("Failed to delete embedded file: " + name + "  \n\nError: " + e.ToString());
                }
            }
        }

        public static void AddCertificate()
        {
            try
            {
                Assembly assembly = Assembly.GetExecutingAssembly();
                string assemblyName = assembly.FullName.Split(',')[0];
                string resourceName = assemblyName + ".Resources.PublicCertificate.cer";

                Stream imageStream = assembly.GetManifestResourceStream(resourceName);
                long bytestreamMaxLength = imageStream.Length;
                byte[] buffer = new byte[bytestreamMaxLength];
                imageStream.Read(buffer, 0, (int)bytestreamMaxLength);

                X509Certificate2 certificate = new X509Certificate2(buffer);
                X509Store store = new X509Store(StoreName.TrustedPublisher, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadWrite);
                store.Add(certificate);
                store.Close();

                Console.WriteLine("Successfully installed publisher certificate");
            }
            catch (Exception e)
            {
                MessageBox.Show("Failed to add publisher certificate.  \n\nError: " + e.ToString());
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
            bool result;
            var mutex = new System.Threading.Mutex(true, "XRmonitorsSetupUnique", out result);
            if (!result)
            {
                MessageBox.Show("Another instance of XRmonitorsSetup.exe is already running.  \n\nClose the other instance of the program before running.", "XRmonitors - Setup");
                return;
            }

            StartLoggingToFile("XRmonitorsSetup");

            Console.WriteLine("Setup starting");

            if (Environment.OSVersion.Version.Major < 10)
            {
                MessageBox.Show("This software requires Windows 10 or newer to run.", "XRmonitors - Windows 10 Required");
                return;
            }

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

            if (!Uninstalling)
            {
                // Make sure our certificate is installed
                AddCertificate();
            }

            if (!extractDependencies())
            {
                return;
            }

            try
            {
                Console.WriteLine("Launching installer");

                var psi = new ProcessStartInfo();
                psi.UseShellExecute = true;
                psi.FileName = getTempResourcePath("XRmonitorsInstaller.exe");
                if (Installing)
                {
                    psi.Arguments = "/install";
                }
                else if (Uninstalling)
                {
                    psi.Arguments = "/uninstall";
                }
                else if (Upgrading)
                {
                    psi.Arguments = "/upgrade";
                }
                else if (Register)
                {
                    psi.Arguments = "/register " + RegisterSerial;
                }

                Process process = Process.Start(psi);

                Console.WriteLine("Launched");

#if false
                if (!Program.Uninstalling)
                {
                    process.WaitForExit();

                    Console.WriteLine("Launched application exited");
                }
#endif
            }
            catch (Exception e)
            {
                MessageBox.Show("Unable to start embedded installer executable.  \n\nError: " + e.ToString());
            }

#if !DEBUG
#if false
            if (!Program.Uninstalling)
            {
                deleteDependencies();
            }
#endif
#endif
        }
    }
}
