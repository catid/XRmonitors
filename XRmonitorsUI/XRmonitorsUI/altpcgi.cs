//
// C# Alternate protection interface implementation class
//
// Version 06.00.0350
//

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Windows.Forms;

class AltPcgi
{
    //
    // interface dll name for 32/64 bit builds
    //
    // set WIN64 for 64bit (x64) builds
    //
    // change default name in case custom interface name is used
    //
//#if WIN64
    const string INTERFACE_DLL = "iDLL64.DLL";
//#else
//    const string INTERFACE_DLL = "iDLL.DLL";
//#endif

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1)]
    public struct PCG_INTERFACE_STRUCT
    {
        public UInt32 PCGI_Size;                // Size of interface structure
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 44)]
        public String PCGI_ApplicationName;     // Application name
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 52)]
        public String PCGI_UserName;            // User name
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 104)]
        public String PCGI_UserAddress;         // User address
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 104)]
        public String PCGI_UserCompany;         // User company    
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 52)]
        public String PCGI_UserCustomInfo1;     // User custom info 1    
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 52)]
        public String PCGI_UserCustomInfo2;     // User custom info 2    
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 52)]
        public String PCGI_UserCustomInfo3;     // User custom info 3    

        public UInt32 PCGI_ProtectionMethod;    // Protection method (0 - REMOTE, 1 - NETWORK, 2 - PLAIN, 3 - CODE)

        public UInt32 PCGI_SiteCode;            // Site code value
        public UInt16 PCGI_MachineID01;            // Machine ID (MID) code
        public UInt16 PCGI_MachineID02;            // 
        public UInt16 PCGI_MachineID03;            // 
        public UInt16 PCGI_MachineID04;            // 
        public UInt32 PCGI_NextSiteCode;        // Next site code value (for limite license feature)
        public UInt16 PCGI_NextMachineID01;        // Next Machine ID (for limited license feature)
        public UInt16 PCGI_NextMachineID02;        // 
        public UInt16 PCGI_NextMachineID03;        // 
        public UInt16 PCGI_NextMachineID04;        // 
        public UInt32 PCGI_ApplicationStatus;    // 0 - Locked, 1 - Unlocked
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 200)]
        public String PCGI_ActivationCode;      // Activation code string

        public UInt32 PCGI_SerialNumbersEnabled;    // Serial numbers feature status (0 - off, 1 - on)
        public UInt32 PCGI_SerialNumberSet;            // Valid serial number set? (0 - no, 1 - yes)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 24)]
        public String PCGI_SerialNumber;            // Serial number string (XXXX-XXXX-XX-XXXX-XXXX)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public String PCGI_SerialNumberFeatures;    // Serial number features

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16, ArraySubType = UnmanagedType.U1)]
        public byte[] PCGI_Features;                  // Custom features
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10, ArraySubType = UnmanagedType.U4)]
        public UInt32[] PCGI_Counters;                // Custom counters

        public UInt32 PCGI_DemoModeActive;            // 0 - Off, 1 - On (application is still in locked state)
        public UInt32 PCGI_DemoDaysLeft;            // Number of days left, 0xFFFFFFFF - Off
        public UInt32 PCGI_DemoUsesLeft;            // Number of days left, 0xFFFFFFFF - Off
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_DemoFixedStartDate;      // Demo mode: Fixed date limitation: Start date (string in DDMMYYYY format)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_DemoFixedExpDate;        // Demo mode: Fixed date limitation: Expiration date (string in DDMMYYYY format)

        public UInt32 PCGI_LimitedLicenseActive;      // 0 - Not active, 1 - Active
        public UInt32 PCGI_LimitedLicenseDaysLeft;      // Number of days left, 0xFFFFFFFF - Off
        public UInt32 PCGI_LimitedLicenseUsesLeft;      // Number of runs left, 0xFFFFFFFF - Off
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_LimLicFixedStartDate;      // Limited license: Fixed date limitation: Start date (string in DDMMYYYY format)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_LimLicFixedExpDate;        // Limited license: Fixed date limitation: Expiration date (string in DDMMYYYY format)

        public UInt32 PCGI_MaxAppInstances;              // Max number of application instances (0 - not set)

        public UInt32 PCGI_MaxNetworkSeats;              // Network protection: max number of workstations allowed to access protected application (0 - not set)
        public UInt32 PCGI_NetCfgFileLoaded;          // Network configuration file loaded (0 - no, 1 - yes)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_NetCfgStartDate;           // Network configuration file start date
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_NetCfgExpDate;             // Network configuration file expiration date

        public UInt32 PCGI_UpdateID;                  // Update ID
        public UInt32 PCGI_UpdatesPolicyDays;          // Updates validity period (in days from activation) (0 - No updates policy)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_UpdatesPolicyStart;        // Updates policy: fixed period: start date (string in DDMMYYYY format)
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 12)]
        public String PCGI_UpdatesPolicyEnd;          // Updates policy: fixed period: end date (string in DDMMYYYY format)

        public UInt32 PCGI_DemoTimer;                  // Current counter of cumulative demo timer (number of minutes of program usage since first run) (05.07.0100)
        public UInt32 PCGI_DemoTimerLimit;            // Cumulative demo timer limit (total allowed time of program usage) (05.07.0100)

        public UInt32 PCGI_LicenseTransfers;          // Number of license transfers made since first activation (05.07.0100)

        public UInt32 PCGI_UpdatesPolicyRangeStart;       // updates policy range start (06.00.0100)
        public UInt32 PCGI_UpdatesPolicyRangeEnd;      // updates policy range end (06.00.0100)

        public UInt32 PCGI_UpdatesPolicyRangeFromStart;// updates policy: "upgrade from" update id range start (06.00.0180)
        public UInt32 PCGI_UpdatesPolicyRangeFromEnd;  // updates policy: "upgrade from" update id range end (06.00.0180)

        public UInt32 PCGI_UpdatesPolicyError;         // 0 - update allowed, 1 - update not allowed (error) (06.00.0200)
        public UInt32 PCGI_FailedUpdateID;             // Update ID of update which failed updates policy (06.00.0200)

        public UInt32 PCGI_LimLicDayOfMonth;           // Limited license, day of the month value (0 means day of the month limitation is off) (06.00.0350)
        public UInt32 PCGI_VirtualMachineFlag;           // Virtual machine flag (0 - VM not detected, 1 - VM detected) (06.00.0350)

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 324)]
        public String PCGI_Reserved;                  // Reserved for future use

    }

    //
    // Each protection interface function is marked with unique Function ID:
    //
    public enum FunctionID
    {
        GetInterfaceData = 1,
        UpdateCustomCounters = 2,
        UnlockApplication = 3,
        RemoveLicense = 4,
        TransferLicense = 5,
        ExtendLicense = 6,
        CheckDemoLimitations = 7,
        CheckLimitedLicense = 8,
        FreeAppInstance = 9,
        CountActiveInstances = 10,
        CountActiveNetworkSeats = 11,
        SetLicenseValidityPeriod = 12,
        SetSerialNumber = 13,
        InvalidateSerialNumber = 14,
        LoadActivationFile = 15,
        CheckForUsbDrive = 16,
        IsVirtualMachine = 17
    };

    //
    // Return codes
    //
    public enum ReturnCodes
    {
        PCGI_STATUS_OK = 0,
        PCGI_ERROR_WRONG_STRUCTURE_SIZE = 1,    // wrong structure size
        PCGI_ERROR_INVALID_ACTIVATION_CODE = 2,    // invalid activation code 
        PCGI_ERROR_ALREADY_UNLOCKED = 3,    // application is already unlocked
        PCGI_SUCCESS_APPLICATION_UNLOCKED = 4,    // application successfully unlocked
        PCGI_SUCCESS_DEMO_EXTENDED = 5,    // demo period successfully extended
        PCGI_ERROR_DEMO_CANT_BE_EXTENDED = 6,    // demo period can't be extended for this application
        PCGI_UNKNOWN_ERROR = 7,    // unknown error
        PCGI_ERROR_LICENSE_CANT_BE_REMOVED = 8,    // license can't be removed
        PCGI_ERROR_APPLICATION_IS_LOCKED = 9,    // application is still in locked state
        PCGI_ERROR_TRANSFER_IMPOSSIBLE = 10,    // license can't be transferred error
        PCGI_ERROR_LICENSE_EXTENSION_ERROR = 11,    // license can't be extended
        PCGI_ERROR_LIMITED_LICENSE_DISABLED = 12,    // limited license is disabled via activation code
        PCGI_ERROR_LICENSE_EXTENSION_DISABLED = 13,    // license extension feature is disabled
        PCGI_ERROR_INVALID_EXTENSION_PERIOD = 14,    // extension period is not valid (0-9999)
        PCGI_ERROR_EVALUATION_PERIOD_EXPIRED = 15,    // evaluation period has expired
        PCGI_ERROR_LICENSE_EXPIRED = 16,    // license has expired
        PCGI_ERROR_DEMO_MODE_DISABLED = 17,    // demo mode disabled
        PCGI_ERROR_LICENSE_UPDATE_DISABLED = 18,    // license update is disabled
        PCGI_ERROR_INVALID_VALIDITY_PERIOD = 19,    // invalid license validity period
        PCGI_ERROR_SERIAL_NUMBERS_DISABLED = 20,    // serial numbers feature is disabled
        PCGI_ERROR_INVALID_SERIAL_NUMBER = 21,    // invalid serial number
        PCGI_ERROR_ACTIVATION_FILE_DISABLED = 22,    // activation file feature is disabled
        PCGI_ERROR_ACTIVATION_FILE_ERROR = 23,    // unable to find, load or process activation file
        PCGI_ERROR_WRONG_PROTECTION_METHOD = 24,// wrong protection method
        PCGI_ERROR_USB_DRIVE_MISSING = 25,        // USB protection: USB drive is not attached to computer
        PCGI_ERROR_SERIAL_NUMBER_NOT_SET = 26,  // serial number is not set
        PCGI_VIRTUAL_MACHINE_DETECTED = 27,        // virtual machine detected
        PCGI_WINDOWS_SYSTEM_ERROR = 28          // Windows system error (Use GetLastError to obtain actual error code)

    };

    //
    // Pointers to protection interface functions
    //
    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 1
    private delegate int Alt_GetInterfaceData(ref PCG_INTERFACE_STRUCT pis);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 2
    private delegate int Alt_UpdateCustomCounters(ref PCG_INTERFACE_STRUCT pis);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 3
    private delegate int Alt_UnlockApplication(String ActivationCode);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 4
    private delegate int Alt_RemoveLicense(ref UInt32 RemovalCode);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 5
    private delegate int Alt_TransferLicense(UInt32 SiteCode, [MarshalAs(UnmanagedType.LPStr)] StringBuilder NewActivationCode);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 6
    private delegate int Alt_ExtendLicense(String ActivationCode);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 7
    private delegate int Alt_CheckDemoLimitations(ref PCG_INTERFACE_STRUCT pis);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 8
    private delegate int Alt_CheckLimitedLicense(ref PCG_INTERFACE_STRUCT pis);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 9
    private delegate int Alt_FreeAppInstance();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 10
    private delegate int Alt_CountActiveInstances();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 11
    private delegate int Alt_CountActiveNetworkSeats();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 12
    private delegate int Alt_SetLicenseValidityPeriod(int Period);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 13
    private delegate int Alt_SetSerialNumber(String SerialNumber);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 14
    private delegate int Alt_InvalidateSerialNumber();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 15
    private delegate int Alt_LoadActivationFile(String ActivationFile);

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 16
    private delegate int Alt_CheckForUsbDrive();

    [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Ansi)]   // 17
    private delegate int Alt_IsVirtualMachine();

    //
    // Required for GetAlternateInterfaceFunction()
    //
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    private static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    private static extern IntPtr GetProcAddress(IntPtr mhandle, string procedureName);

    private delegate IntPtr GetAltFunctionAddress(int functionID);

    private static Delegate GetAlternateInterfaceFunction(int functionID, Type fType)
    {
        // Get interface dll module handle
        IntPtr hIdll = GetModuleHandle(INTERFACE_DLL);

        // If not loaded, return error
        if (hIdll == IntPtr.Zero)
        {
            MessageBox.Show("Interface DLL is not loaded?");
            return null;
        }

        // Get I10 function address
        IntPtr I10 = GetProcAddress(hIdll, "I10");

        // If not found, return error
        if (hIdll == IntPtr.Zero)
        {
            MessageBox.Show("Alternate interface function can not be found?");
            return null;
        }

        GetAltFunctionAddress GetAltFunctionAddress = (GetAltFunctionAddress) Marshal.GetDelegateForFunctionPointer((IntPtr)I10, typeof(GetAltFunctionAddress));

        IntPtr fAddress = (IntPtr)GetAltFunctionAddress(functionID);

        if (fAddress == null) return null;

        return Marshal.GetDelegateForFunctionPointer(fAddress, fType);
    }

    /// <summary>
    /// Function will retrieve information about current license from protection code.
    /// </summary>
    /// <param name="pis">PCG_INTERFACE_STRUCT to be filled with data</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int GetInterfaceData(ref PCG_INTERFACE_STRUCT pis)
    {
        pis.PCGI_Size = (UInt32)Marshal.SizeOf(pis);

        Alt_GetInterfaceData Alt_GetInterfaceData = (Alt_GetInterfaceData)GetAlternateInterfaceFunction((int)FunctionID.GetInterfaceData, typeof(Alt_GetInterfaceData));

        if (Alt_GetInterfaceData == null) return -1;

        return Alt_GetInterfaceData(ref pis);
    }

    /// <summary>
    /// Function will instruct protection code to save current state of counters
    /// </summary>
    /// <param name="pis">PCG_INTERFACE_STRUCT with custom counters</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int UpdateCustomCounters(ref PCG_INTERFACE_STRUCT pis)
    {
        pis.PCGI_Size = (UInt32)Marshal.SizeOf(pis);

        Alt_UpdateCustomCounters Alt_UpdateCustomCounters = (Alt_UpdateCustomCounters)GetAlternateInterfaceFunction((int)FunctionID.UpdateCustomCounters, typeof(Alt_UpdateCustomCounters));

        if (Alt_UpdateCustomCounters == null) return -1;

        return Alt_UpdateCustomCounters(ref pis);
    }


    /// <summary>
    /// Function will activate application
    /// </summary>
    /// <param name="ActivationCode">Activation code string</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int UnlockApplication(String ActivationCode)
    {
        Alt_UnlockApplication Alt_UnlockApplication = (Alt_UnlockApplication)GetAlternateInterfaceFunction((int)FunctionID.UnlockApplication, typeof(Alt_UnlockApplication));

        if (Alt_UnlockApplication == null) return -1;

        return Alt_UnlockApplication(ActivationCode);
    }

    /// <summary>
    /// Function will remove license
    /// </summary>
    /// <returns>Function returns removal code or empty string in case of error</returns>
    public static string RemoveLicense()
    {
        UInt32 RemovalCode = 0;

        Alt_RemoveLicense Alt_RemoveLicense = (Alt_RemoveLicense)GetAlternateInterfaceFunction((int)FunctionID.RemoveLicense, typeof(Alt_RemoveLicense));

        if (Alt_RemoveLicense == null) return "";

        int retValue = Alt_RemoveLicense(ref RemovalCode);

        return ((retValue == 0) ? RemovalCode.ToString("X8") : "");
    }

    /// <summary>
    /// Function will transfer active license to another computer.
    /// </summary>
    /// <param name="SiteCode">new Site code value</param>
    /// <param name="NewActivationCode">pointer to new Activation code buffer</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int TransferLicense(UInt32 SiteCode, ref String NewActivationCode)
    {
        StringBuilder sb = new StringBuilder(200);

        Alt_TransferLicense Alt_TransferLicense = (Alt_TransferLicense)GetAlternateInterfaceFunction((int)FunctionID.TransferLicense, typeof(Alt_TransferLicense));

        if (Alt_TransferLicense == null) return -1;

        int retVal = Alt_TransferLicense(SiteCode, sb);

        if (retVal == 0) NewActivationCode = sb.ToString();

        return retVal;
    }

    /// <summary>
    /// Function will update license with new activation code data.
    /// </summary>
    /// <param name="ActivationCode">Activation code string</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int ExtendLicense(String ActivationCode)
    {
        Alt_ExtendLicense Alt_ExtendLicense = (Alt_ExtendLicense)GetAlternateInterfaceFunction((int)FunctionID.ExtendLicense, typeof(Alt_ExtendLicense));

        if (Alt_ExtendLicense == null) return -1;

        return Alt_ExtendLicense(ActivationCode);
    }

    /// <summary>
    /// Function will check demo (date and fixed date) limitations.
    /// </summary>
    /// <param name="pis">PCG_INTERFACE_STRUCT to be filled with data</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int CheckDemoLimitations(ref PCG_INTERFACE_STRUCT pis)
    {
        pis.PCGI_Size = (UInt32)Marshal.SizeOf(pis);

        Alt_CheckDemoLimitations Alt_CheckDemoLimitations = (Alt_CheckDemoLimitations)GetAlternateInterfaceFunction((int)FunctionID.CheckDemoLimitations, typeof(Alt_CheckDemoLimitations));

        if (Alt_CheckDemoLimitations == null) return -1;

        return Alt_CheckDemoLimitations(ref pis);
    }

    /// <summary>
    /// Function will check limited license expiration status.
    /// </summary>
    /// <param name="pis">PCG_INTERFACE_STRUCT to be filled with data</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int CheckLimitedLicense(ref PCG_INTERFACE_STRUCT pis)
    {
        pis.PCGI_Size = (UInt32)Marshal.SizeOf(pis);

        Alt_CheckLimitedLicense Alt_CheckLimitedLicense = (Alt_CheckLimitedLicense)GetAlternateInterfaceFunction((int)FunctionID.CheckLimitedLicense, typeof(Alt_CheckLimitedLicense));

        if (Alt_CheckLimitedLicense == null) return -1;

        return Alt_CheckLimitedLicense(ref pis);
    }

    /// <summary>
    /// Function will free current application instance
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int FreeAppInstance()
    {
        Alt_FreeAppInstance Alt_FreeAppInstance = (Alt_FreeAppInstance)GetAlternateInterfaceFunction((int)FunctionID.FreeAppInstance, typeof(Alt_FreeAppInstance));

        if (Alt_FreeAppInstance == null) return -1;

        return Alt_FreeAppInstance();
    }

    /// <summary>
    /// Function will return number of currently active application instances.
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int CountActiveInstances()
    {
        Alt_CountActiveInstances Alt_CountActiveInstances = (Alt_CountActiveInstances)GetAlternateInterfaceFunction((int)FunctionID.CountActiveInstances, typeof(Alt_CountActiveInstances));

        if (Alt_CountActiveInstances == null) return -1;

        return Alt_CountActiveInstances();
    }

    /// <summary>
    /// Function function will return number of currently active network seats.
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int CountActiveNetworkSeats()
    {
        Alt_CountActiveNetworkSeats Alt_CountActiveNetworkSeats = (Alt_CountActiveNetworkSeats)GetAlternateInterfaceFunction((int)FunctionID.CountActiveNetworkSeats, typeof(Alt_CountActiveNetworkSeats));

        if (Alt_CountActiveNetworkSeats == null) return -1;

        return Alt_CountActiveNetworkSeats();
    }


    /// <summary>
    /// Function will set license validity period. 
    /// </summary>
    /// <param name="Period">New license validity period in days (1-9999)</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int SetLicenseValidityPeriod(int Period)
    {
        Alt_SetLicenseValidityPeriod Alt_SetLicenseValidityPeriod = (Alt_SetLicenseValidityPeriod)GetAlternateInterfaceFunction((int)FunctionID.SetLicenseValidityPeriod, typeof(Alt_SetLicenseValidityPeriod));

        if (Alt_SetLicenseValidityPeriod == null) return -1;

        return Alt_SetLicenseValidityPeriod(Period);
    }

    /// <summary>
    /// Function will validate and set serial number for protected application.
    /// </summary>
    /// <param name="SerialNumber">Pointer to serial number string</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int SetSerialNumber(String SerialNumber)
    {
        Alt_SetSerialNumber Alt_SetSerialNumber = (Alt_SetSerialNumber)GetAlternateInterfaceFunction((int)FunctionID.SetSerialNumber, typeof(Alt_SetSerialNumber));

        if (Alt_SetSerialNumber == null) return -1;

        return Alt_SetSerialNumber(SerialNumber);
    }

    /// <summary>
    /// Function will invalidate existing serial number and reset license. 
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int InvalidateSerialNumber()
    {
        Alt_InvalidateSerialNumber Alt_InvalidateSerialNumber = (Alt_InvalidateSerialNumber)GetAlternateInterfaceFunction((int)FunctionID.InvalidateSerialNumber, typeof(Alt_InvalidateSerialNumber));

        if (Alt_InvalidateSerialNumber == null) return -1;

        return Alt_InvalidateSerialNumber();
    }

    /// <summary>
    /// Function will load and process activation file and thus activate application or extend evaluation period 
    /// </summary>
    /// <param name="ActivationFile">full path to activation file</param>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int LoadActivationFile(String ActivationFile)
    {
        Alt_LoadActivationFile Alt_LoadActivationFile = (Alt_LoadActivationFile)GetAlternateInterfaceFunction((int)FunctionID.LoadActivationFile, typeof(Alt_LoadActivationFile));

        if (Alt_LoadActivationFile == null) return -1;

        return Alt_LoadActivationFile(ActivationFile);
    }

    /// <summary>
    /// Function will check if required USB drive is attached to computer 
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int CheckForUsbDrive()
    {
        Alt_CheckForUsbDrive Alt_CheckForUsbDrive = (Alt_CheckForUsbDrive)GetAlternateInterfaceFunction((int)FunctionID.CheckForUsbDrive, typeof(Alt_CheckForUsbDrive));

        if (Alt_CheckForUsbDrive == null) return -1;

        return Alt_CheckForUsbDrive();
    }

    /// <summary>
    /// Function will check if application is running under virtual machine (VM)
    /// </summary>
    /// <returns>Returns status code or -1 in case interface dll is not loaded.</returns>
    public static int IsVirtualMachine()
    {
        Alt_IsVirtualMachine Alt_IsVirtualMachine = (Alt_IsVirtualMachine)GetAlternateInterfaceFunction((int)FunctionID.IsVirtualMachine, typeof(Alt_IsVirtualMachine));

        if (Alt_IsVirtualMachine == null) return -1;

        return Alt_IsVirtualMachine();
    }

}
