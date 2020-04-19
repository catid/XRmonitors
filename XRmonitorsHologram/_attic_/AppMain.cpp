// AppMain.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AppMain.h"

#include <HolographicSpaceInterop.h>
#include <windows.graphics.holographic.h>
#include <winrt\Windows.Graphics.Holographic.h>

#include "core_logger.hpp"
#include "core_string.hpp"
#include "core_win32.hpp"
#include "core_serializer.hpp"
using namespace core;

#pragma comment(lib, "Imm32")

static logger::Channel Logger("Main");

using namespace XRmonitorsHologram;

using namespace concurrency;
using namespace std::placeholders;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Graphics::Holographic;
using namespace winrt::Windows::UI::Core;

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    winrt::init_apartment();

    App app;

    // Initialize global strings, and perform application initialization.
    app.Initialize(hInstance);

    // Create the HWND and the HolographicSpace.
    app.CreateWindowAndHolographicSpace(hInstance, nCmdShow);

    // Main message loop:
    app.Run(hInstance);

    // Perform application teardown.
    app.Uninitialize();

    return 0;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM App::MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXA wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = &this->WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MonitorHologramMAIN));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEA(IDC_MonitorHologramMAIN);
    wcex.lpszClassName = m_szWindowClass;
    wcex.hIconSm = NULL;

    return ::RegisterClassExA(&wcex);
}

// The first method called when the IFrameworkView is being created.
// Use this method to subscribe for Windows shell events and to initialize your app.
void App::Initialize(HINSTANCE hInstance)
{
    // Initialize global strings
    //LoadStringW(hInstance, IDS_APP_TITLE, m_szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_WINDOWSPROJECT5, m_szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // At this point we have access to the device and we can create device-dependent
    // resources.
    m_deviceResources = std::make_shared<DX::DeviceResources>();

    m_main = std::make_unique<MonitorHologramMain>(&Cameras, m_deviceResources);
}


//
//   FUNCTION: CreateWindowAndHolographicSpace(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle, creates main window, and creates HolographicSpace
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create the main program window. We also create the HolographicSpace.
//
void App::CreateWindowAndHolographicSpace(HINSTANCE hInstance, int nCmdShow)
{
    // Store the instance handle in our class variable.
    m_hInst = hInstance;

    // Create the window for the HolographicSpace.
    hWnd = ::CreateWindowExA(
		0,
        m_szWindowClass, 
        m_szTitle,
        0/*WS_VISIBLE*/,
        CW_USEDEFAULT, 
        0, 
        CW_USEDEFAULT, 
        0, 
        nullptr, 
        nullptr, 
        hInstance, 
        nullptr);

    if (!hWnd)
    {
        winrt::check_hresult(E_FAIL);
    }

    {
        // Use WinRT factory to create the holographic space.
        using namespace winrt::Windows::Graphics::Holographic;
        winrt::com_ptr<IHolographicSpaceInterop> holographicSpaceInterop = winrt::get_activation_factory<HolographicSpace, IHolographicSpaceInterop>();
        winrt::com_ptr<ABI::Windows::Graphics::Holographic::IHolographicSpace> spHolographicSpace;
        winrt::check_hresult(holographicSpaceInterop->CreateForWindow(hWnd, __uuidof(ABI::Windows::Graphics::Holographic::IHolographicSpace), winrt::put_abi(spHolographicSpace)));

        if (!spHolographicSpace)
        {
            winrt::check_hresult(E_FAIL);
        }

        // Store the holographic space.
        m_holographicSpace = spHolographicSpace.as<HolographicSpace>();
    }

    // The DeviceResources class uses the preferred DXGI adapter ID from the holographic
    // space (when available) to create a Direct3D device. The HolographicSpace
    // uses this ID3D11Device to create and manage device-based resources such as
    // swap chains.
    m_deviceResources->SetHolographicSpace(m_holographicSpace);

    // The main class uses the holographic space for updates and rendering.
    m_main->SetHolographicSpace(hWnd, m_holographicSpace);

    // Show the window. This will activate the holographic view and switch focus to the app in Windows Mixed Reality.
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK App::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        OutputDebugStringA("Destroy\n");
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

#define HOTKEY1_RESET WM_USER + 1
#define HOTKEY2_INCREASE_DPI WM_USER + 2
#define HOTKEY3_DECREASE_DPI WM_USER + 3

#if 0

static BOOL CALLBACK winy_enum(HWND hWnd, LPARAM lParam)
{
	int length = ::GetWindowTextLengthA(hWnd);
	if (length <= 0 || length > 2000) {
		return TRUE;
	}

	DWORD pid = 0;
	::GetWindowThreadProcessId(hWnd, &pid);
	if (pid != 16288) {
		return TRUE;
	}

	HWND focus = ::GetFocus();
	Logger.Info("focus: ", HexString((uintptr_t)focus));
	HWND active = ::GetActiveWindow();
	Logger.Info("active: ", HexString((uintptr_t)active));
	HWND foreground = ::GetForegroundWindow();
	Logger.Info("foreground: ", HexString((uintptr_t)foreground));

#if 0
#define IMMGWLP_IMC 0
	const HIMC imc = reinterpret_cast<HIMC>(::GetWindowLongPtrA(hWnd, IMMGWLP_IMC));
	//HIMC imc = ::ImmGetContext(hWnd);
	Logger.Info("IMC = ", imc);
	if (imc)
	{
		BOOL open = ::ImmGetOpenStatus(imc);
		Logger.Info("OPEN = ", open);
	}
#endif
	std::vector<char> buffer_title(length + 1);
	::GetWindowTextA(hWnd, &buffer_title[0], length + 1);
	buffer_title[length] = '\0';
	const char* name = &buffer_title[0];

#if 0
	const char* target_name = "Windows Mixed Reality";
	if (StrCaseCompare(name, target_name) != 0) {
		return TRUE;
	}
#endif

	char class_name[100];
	int class_result = ::GetClassNameA(hWnd, class_name, sizeof(class_name));
	if (class_result == 0) {
		Logger.Error("GetClassNameA failed: ", ::GetLastError());
		return TRUE;
	}
	class_name[sizeof(class_name) - 1] = '\0';

#if 0
	const char* target_class = "Windows.UI.Core.CoreWindow";
	if (StrCaseCompare(class_name, target_class) != 0) {
		return TRUE;
	}
#endif

	Logger.Info("Pid: ", pid);
	Logger.Info("Window: ", name);
	Logger.Info("Class: ", class_name);
	Logger.Info("hWnd: ", HexString((uintptr_t)hWnd));

	WINDOWINFO wi;
	wi.cbSize = sizeof(wi);
	BOOL wi_result = ::GetWindowInfo(hWnd, &wi);

	if (!wi_result)
	{
		Logger.Error("GetWindowInfo failed: ", ::GetLastError());
		return TRUE;
	}

	Logger.Info("wi.rcWindow.left = ", wi.rcWindow.left);
	Logger.Info("wi.rcWindow.right = ", wi.rcWindow.right);
	Logger.Info("wi.rcWindow.top = ", wi.rcWindow.top);
	Logger.Info("wi.rcWindow.bottom = ", wi.rcWindow.bottom);

	Logger.Info("wi.rcClient.left = ", wi.rcClient.left);
	Logger.Info("wi.rcClient.right = ", wi.rcClient.right);
	Logger.Info("wi.rcClient.top = ", wi.rcClient.top);
	Logger.Info("wi.rcClient.bottom = ", wi.rcClient.bottom);

	bool* result_ptr = (bool*)lParam;

	//dwExStyle = 538970112
	//dwStyle = 2483027968
	//dwWindowStatus = 0
	//dwExStyle = 538970112
	//dwStyle = 2483027968
	//dwWindowStatus = 0
	//dwExStyle = 538970112
	//dwStyle = 2483027968
	//dwWindowStatus = 0

	Logger.Info("dwExStyle = ", wi.dwExStyle);
	Logger.Info("dwStyle = ", wi.dwStyle);
	Logger.Info("dwWindowStatus = ", wi.dwWindowStatus);

	return TRUE;
}

bool IsWinYPromptOpen()
{
	bool result = false;
	::EnumWindows(winy_enum, (LPARAM)& result);
	return result;
}

#endif

void DismissWinY()
{
	Logger.Info("Attempting to dismiss Win+Y");

#if 0
	if (!IsWinYPromptOpen()) {
		return;
	}
#endif

	keybd_event(
		VK_LWIN,
		MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC),
		KEYEVENTF_EXTENDEDKEY,
		0);
	keybd_event(
		VkKeyScan('Y'),
		MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC), // Y
		0,
		0);
	keybd_event(
		VK_LWIN,
		MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC),
		KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
		0);
	keybd_event(
		VkKeyScan('Y'),
		MapVirtualKeyA(VK_LWIN, MAPVK_VK_TO_VSC), // Y
		KEYEVENTF_KEYUP,
		0);
}



//
//  FUNCTION: Run(HINSTANCE hInstance)
//
//  PURPOSE: Runs the Windows message loop and game loop.
//
int App::Run(HINSTANCE hInstance)
{
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MonitorHologramMAIN));

    MSG msg { };

	bool old_doff = true;

	::RegisterHotKey(nullptr, HOTKEY1_RESET, MOD_WIN | MOD_NOREPEAT, VK_OEM_3);
	::RegisterHotKey(nullptr, HOTKEY2_INCREASE_DPI, MOD_WIN | MOD_NOREPEAT, VK_PRIOR);
	::RegisterHotKey(nullptr, HOTKEY3_DECREASE_DPI, MOD_WIN | MOD_NOREPEAT, VK_NEXT);

	bool hotkey1_down = false;
	bool hotkey2_down = false;
	bool hotkey3_down = false;

	bool doff_first = true;

	uint64_t DismissTriggerTicks = 0;

	//DismissWinY();

	XrmUiData ui_prev_data;

    // Main message loop
    bool isRunning = true;
    while (isRunning)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                isRunning = false;
            }
			else if (msg.message == WM_HOTKEY)
            {
				if (msg.wParam == HOTKEY1_RESET) {
					if (!hotkey1_down) {
						hotkey1_down = true;
						m_main->OnHotkey(1, true);
					}
				}
				else if (msg.wParam == HOTKEY2_INCREASE_DPI) {
					if (!hotkey2_down) {
						hotkey2_down = true;
						m_main->OnHotkey(2, true);
					}
				}
				else if (msg.wParam == HOTKEY3_DECREASE_DPI) {
					if (!hotkey3_down) {
						hotkey3_down = true;
						m_main->OnHotkey(3, true);
					}
				}
			}
			else if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
			if (hotkey1_down) {
				if ((::GetAsyncKeyState(VK_OEM_3) >> 15) == 0) {
					hotkey1_down = false;
					m_main->OnHotkey(1, false);
				}
			}
			if (hotkey2_down) {
				if ((::GetAsyncKeyState(VK_PRIOR) >> 15) == 0) {
					hotkey2_down = false;
					m_main->OnHotkey(2, false);
				}
			}
			if (hotkey3_down) {
				if ((::GetAsyncKeyState(VK_NEXT) >> 15) == 0) {
					hotkey3_down = false;
					m_main->OnHotkey(3, false);
				}
			}

#if 0
			HWND hwnd = ::GetFocus();
			char title[256];
			::GetWindowTextA(hwnd, title, sizeof(title));
			Logger.Info("TEST: ", title);
#endif

			XrmUiData ui_data;
			Cameras.ReadUiState(ui_data);

			if (ui_data.RecenterKeys[0] != 0 &&
				0 != memcmp(&ui_prev_data.RecenterKeys, &ui_data.RecenterKeys, sizeof(ui_data.RecenterKeys)))
			{
				Logger.Info("RecenterKeys shortcut updated");
			}

			if (ui_data.DecreaseKeys[0] != 0 &&
				0 != memcmp(&ui_prev_data.DecreaseKeys, &ui_data.DecreaseKeys, sizeof(ui_data.DecreaseKeys)))
			{
				Logger.Info("DecreaseKeys shortcut updated");
			}

			if (ui_data.IncreaseKeys[0] != 0 &&
				0 != memcmp(&ui_prev_data.IncreaseKeys, &ui_data.IncreaseKeys, sizeof(ui_data.IncreaseKeys)))
			{
				Logger.Info("IncreaseKeys shortcut updated");
			}

			ui_prev_data = ui_data;

            if (m_windowVisible && (m_holographicSpace != nullptr))
            {
				bool doff = (m_holographicSpace.UserPresence() == HolographicSpaceUserPresence::Absent);

				// Microsoft Bug: If headset is on the user's head as they put it on,
				// it will report Absent here until they take it off and put it back on.
				if (!doff) {
					doff_first = false;
				}

				if (doff != old_doff) {
					old_doff = doff;
					Logger.Info("Doff: ", doff);

					if (!doff) {
						if (ui_data.DisableWinY) {
							DismissTriggerTicks = ::GetTickCount64() + 250;
							Logger.Info("Delay trigger on Win+Y, waiting for HoloApp to go first.");
						}
					}

					if (!doff_first) {
						m_main->OnDoff(doff);
					}
				}

				if (!doff_first && doff) {
					continue;
				}

				if (DismissTriggerTicks != 0 &&
					(int64_t)(::GetTickCount64() - DismissTriggerTicks) > 0)
				{
					Logger.Info("Triggering: Dismiss Win+Y");
					DismissTriggerTicks = 0;
					DismissWinY();
				}

				HolographicFramePrediction prediction = nullptr;
                HolographicFrame holographicFrame = m_main->Update(prediction);

				if (!m_main->Render(holographicFrame, prediction)) {
					Sleep(10);
					continue;
				}

				// The holographic frame has an API that presents the swap chain for each
                // HolographicCamera.
                m_deviceResources->Present(holographicFrame);
            }
        }
    }

	::UnregisterHotKey(nullptr, HOTKEY1_RESET);
	::UnregisterHotKey(nullptr, HOTKEY2_INCREASE_DPI);
	::UnregisterHotKey(nullptr, HOTKEY3_DECREASE_DPI);

    return (int)msg.wParam;
}

//
//  FUNCTION: Uninitialize()
//
//  PURPOSE: Tear down app resources.
//
void App::Uninitialize()
{
    m_main.reset();
    m_deviceResources.reset();
}
