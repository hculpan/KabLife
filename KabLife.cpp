// Windows Header Files:
#include <windows.h>

// C RunTime Header Files:
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <time.h>
#include <process.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

template<class Interface>
inline void SafeRelease(
    Interface** ppInterfaceToRelease
)
{
    if (*ppInterfaceToRelease != NULL)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = NULL;
    }
}


#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while(0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif



#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define GRID_WIDTH 189
#define GRID_HEIGHT 130

class DemoApp
{
public:
    DemoApp();
    ~DemoApp();

    // Register the window class and call methods for instantiating drawing resources
    HRESULT Initialize();

    // Process and dispatch messages
    void RunMessageLoop();

private:
    BOOL* m_cell;

    BOOL m_cell1[GRID_WIDTH * GRID_HEIGHT];
    BOOL m_cell2[GRID_WIDTH * GRID_HEIGHT];

    // Initialize device-independent resources.
    HRESULT CreateDeviceIndependentResources();

    // Initialize device-dependent resources.
    HRESULT CreateDeviceResources();

    // Release device-dependent resource.
    void DiscardDeviceResources();

    // Draw content.
    HRESULT OnRender();

    // Resize the render target.
    void OnResize(
        UINT width,
        UINT height
    );

    static void ProcessProc(void *ptr);

    static UINT CountNeighbors(UINT x, UINT y, BOOL *cell);

    // The windows procedure.
    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    HANDLE m_hRunMutex;
    HWND m_hwnd;
    ID2D1Factory* m_pDirect2dFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
    ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
};

DemoApp::DemoApp() :
    m_hwnd(NULL),
    m_pDirect2dFactory(NULL),
    m_pRenderTarget(NULL),
    m_pLightSlateGrayBrush(NULL),
    m_pCornflowerBlueBrush(NULL)
{
    m_cell = &m_cell1[0];

    m_hRunMutex = CreateMutexW(NULL, TRUE, NULL);      // Set
    
    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            if (rand() % 100 > 50) m_cell[(y * GRID_WIDTH) + x] = TRUE;
            else m_cell[(y * GRID_WIDTH) + x] = FALSE;
        }
    }
}


DemoApp::~DemoApp()
{
    SafeRelease(&m_pDirect2dFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pLightSlateGrayBrush);
    SafeRelease(&m_pCornflowerBlueBrush);

}

void DemoApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

HRESULT DemoApp::Initialize()
{
    HRESULT hr;

    // Initialize device-indpendent resources, such
    // as the Direct2D factory.
    hr = CreateDeviceIndependentResources();

    if (SUCCEEDED(hr))
    {
        // Register the window class.
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = DemoApp::WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = sizeof(LONG_PTR);
        wcex.hInstance = HINST_THISCOMPONENT;
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = NULL;
        wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
        wcex.lpszClassName = L"KabLife";

        RegisterClassEx(&wcex);

        // The factory returns the current system DPI. This is also the value it will use
        // to create its own windows.
        const HWND hDesktop = GetDesktopWindow();
        UINT dpi = GetDpiForWindow(hDesktop);

        // Create the window.
        m_hwnd = CreateWindow(
            L"KabLife",
            L"KabLife",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil(1272 * dpi / 96)),
            static_cast<UINT>(ceil(895 * dpi / 96)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
        );
        hr = m_hwnd ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            ShowWindow(m_hwnd, SW_SHOWNORMAL);
            UpdateWindow(m_hwnd);
        }
    }

    return hr;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = m_pDirect2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_pRenderTarget
        );


        if (SUCCEEDED(hr))
        {
            // Create a gray brush.
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::LightSlateGray),
                &m_pLightSlateGrayBrush
            );
        }
        if (SUCCEEDED(hr))
        {
            // Create a blue brush.
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::CornflowerBlue),
                &m_pCornflowerBlueBrush
            );
        }
    }

    return hr;
}

void DemoApp::DiscardDeviceResources()
{
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pLightSlateGrayBrush);
    SafeRelease(&m_pCornflowerBlueBrush);
}

void DemoApp::OnResize(UINT width, UINT height)
{
    if (m_pRenderTarget)
    {
        // Note: This method can fail, but it's okay to ignore the
        // error here, because the error will be returned again
        // the next time EndDraw is called.
        m_pRenderTarget->Resize(D2D1::SizeU(width, height));
    }
}

UINT DemoApp::CountNeighbors(UINT x, UINT y, BOOL *cell) {
    UINT result = 0;

    if (y > 0) {
        if (x > 0 && cell[((y - 1) * GRID_WIDTH) + x - 1]) result++;
        if (cell[((y - 1) * GRID_WIDTH) + x]) result++;
        if (x + 1 < GRID_WIDTH && cell[((y - 1) * GRID_WIDTH) + x + 1]) result++;
    }

    if (x > 0 && cell[(y * GRID_WIDTH) + x - 1]) result++;
    if (x + 1 < GRID_WIDTH && cell[(y * GRID_WIDTH) + x + 1]) result++;

    if (y + 1 < GRID_HEIGHT) {
        if (x > 0 && cell[((y + 1) * GRID_WIDTH) + x - 1]) result++;
        if (y > 0 && cell[((y + 1) * GRID_WIDTH) + x]) result++;
        if (x + 1 < GRID_WIDTH && cell[((y + 1) * GRID_WIDTH) + x + 1]) result++;
    }
    return result;
}

void DemoApp::ProcessProc(void *ptr)
{
    DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(ptr);

    BOOL* oldGrid = &pDemoApp->m_cell1[0];
    BOOL* newGrid = &pDemoApp->m_cell2[0];
    BOOL* tempGrid;

    UINT neighbors;

    do {
        if (pDemoApp->m_cell == &pDemoApp->m_cell1[0]) {
            oldGrid = &pDemoApp->m_cell1[0];
            newGrid = &pDemoApp->m_cell2[0];
        }
        else {
            oldGrid = &pDemoApp->m_cell2[0];
            newGrid = &pDemoApp->m_cell1[0];
        }

        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                neighbors = DemoApp::CountNeighbors(x, y, oldGrid);
                BOOL alive = oldGrid[(y * GRID_WIDTH) + x];

                if ((alive && (neighbors == 2 || neighbors == 3)) || (!alive && neighbors == 3)) {
                    newGrid[(y * GRID_WIDTH) + x] = TRUE;
                }
                else {
                    newGrid[(y * GRID_WIDTH) + x] = FALSE;
                }
            }
        }

        pDemoApp->m_cell = newGrid;
        tempGrid = oldGrid;
        oldGrid = newGrid;
        newGrid = tempGrid;

        InvalidateRect(pDemoApp->m_hwnd, NULL, FALSE);
    }
    while (WaitForSingleObject(pDemoApp->m_hRunMutex, 75L) == WAIT_TIMEOUT);
}

HRESULT DemoApp::OnRender()
{
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();
    if (SUCCEEDED(hr))
    {
        BOOL* cell = m_cell;

        m_pRenderTarget->BeginDraw();

        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        D2D1_SIZE_F rtSize = m_pRenderTarget->GetSize();

        // Draw a grid background.
        int width = static_cast<int>(rtSize.width);
        int height = static_cast<int>(rtSize.height);

        for (int x = 0; x < width; x += 10)
        {
            m_pRenderTarget->DrawLine(
                D2D1::Point2F(static_cast<FLOAT>(x + 1), 0.0f),
                D2D1::Point2F(static_cast<FLOAT>(x + 1), rtSize.height),
                m_pLightSlateGrayBrush,
                0.5f
            );
        }

        for (int y = 0; y < height; y += 10)
        {
            m_pRenderTarget->DrawLine(
                D2D1::Point2F(0.0f, static_cast<FLOAT>(y + 1)),
                D2D1::Point2F(rtSize.width, static_cast<FLOAT>(y + 1)),
                m_pLightSlateGrayBrush,
                0.5f
            );
        }

        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                if (cell[(y * GRID_WIDTH) + x]) {
                    D2D1_RECT_F rectangle1 = D2D1::RectF(
                        (x * 10) + 2,
                        (y * 10) + 2,
                        (x * 10) + 10,
                        (y * 10) + 10
                    );
                    m_pRenderTarget->FillRectangle(&rectangle1, m_pCornflowerBlueBrush);
                }
            }
        }

        hr = m_pRenderTarget->EndDraw();

        if (hr == D2DERR_RECREATE_TARGET)
        {
            hr = S_OK;
            DiscardDeviceResources();
        }
    }

    return hr;
}

LRESULT CALLBACK DemoApp::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

        DemoApp* pDemoApp = (DemoApp*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pDemoApp)
        );

        _beginthread(DemoApp::ProcessProc, 0, pDemoApp);

        result = 1;
    }
    else
    {
        DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pDemoApp)
        {
            switch (message)
            {
            case WM_SIZE:
            {
                UINT width = LOWORD(lParam);
                UINT height = HIWORD(lParam);
                pDemoApp->OnResize(width, height);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DISPLAYCHANGE:
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_PAINT:
            {
                pDemoApp->OnRender();
                ValidateRect(hwnd, NULL);
            }
            result = 0;
            wasHandled = true;
            break;

            case WM_DESTROY:
            {
                PostQuitMessage(0);
                ReleaseMutex(pDemoApp->m_hRunMutex);
            }
            result = 1;
            wasHandled = true;
            break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}

int WINAPI WinMain(
    HINSTANCE /* hInstance */,
    HINSTANCE /* hPrevInstance */,
    LPSTR /* lpCmdLine */,
    int /* nCmdShow */
)
{
    srand(time(NULL));
    
    // Use HeapSetInformation to specify that the process should
    // terminate if the heap manager detects an error in any heap used
    // by the process.
    // The return value is ignored, because we want to continue running in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            DemoApp app;

            if (SUCCEEDED(app.Initialize()))
            {
                app.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}