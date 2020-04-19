#pragma once

#include "resource.h"

#include "Common/DeviceResources.h"
#include "MonitorHologramMain.h"

namespace XRmonitorsHologram
{
    // IFrameworkView class. Connects the app with the Windows shell and handles application lifecycle events.
    class App sealed
    {
    public:
        void Initialize(HINSTANCE hInstance);
        void CreateWindowAndHolographicSpace(HINSTANCE hInstance, int nCmdShow);
        int  Run(HINSTANCE hInstance);
        void Uninitialize();

        static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    private:
        ATOM MyRegisterClass(HINSTANCE hInstance);

        std::unique_ptr<MonitorHologramMain> m_main;

		core::CameraClient Cameras;

        HWND hWnd;

        std::shared_ptr<DX::DeviceResources>                    m_deviceResources;
        bool                                                    m_windowClosed = false;
        bool                                                    m_windowVisible = true;

        HINSTANCE m_hInst;                                // current instance
        const char* m_szTitle = "XRMonitors";
        const char* m_szWindowClass = "XRMonitorsClass"; // The title bar text

        // The holographic space the app will use for rendering.
        winrt::Windows::Graphics::Holographic::HolographicSpace m_holographicSpace = nullptr;
    };
}
