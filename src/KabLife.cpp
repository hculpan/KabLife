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
    UINT GridWidth = 180;
    UINT GridHeight = 120;

    BOOL* m_cell;

    BOOL* m_cell1;
    BOOL* m_cell2;

    UINT m_iterationCount = 0;

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

    static void OnStartButton(DemoApp* pDemoApp);

    static void ProcessProc(void *ptr);

    UINT CountNeighbors(UINT x, UINT y, BOOL *cell);

    inline UINT CellIndex(UINT x, UINT y);

    // The windows procedure.
    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

    HANDLE m_hRunMutex;
    HWND m_hwndParent;
    HWND m_hwndRenderTarget;
    HWND m_hwndStartButton;

    // Direct2D objects
    ID2D1Factory* m_pDirect2dFactory;
    ID2D1HwndRenderTarget* m_pRenderTarget;
    ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
    ID2D1SolidColorBrush* m_pCornflowerBlueBrush;

    // Text objects
    IDWriteFactory* m_pDWriteFactory;
    IDWriteTextFormat* m_pTextFormat;
    ID2D1Factory* m_pD2DFactory;
};

DemoApp::DemoApp() :
    m_hwndParent(NULL),
    m_pDirect2dFactory(NULL),
    m_pRenderTarget(NULL),
    m_pLightSlateGrayBrush(NULL),
    m_pCornflowerBlueBrush(NULL)
{
    m_cell1 = (BOOL*)(malloc(GridWidth * GridHeight * sizeof(BOOL)));
    m_cell2 = (BOOL*)(malloc(GridWidth * GridHeight * sizeof(BOOL)));

    m_cell = NULL;
}

DemoApp::~DemoApp()
{
    SafeRelease(&m_pDirect2dFactory);
    SafeRelease(&m_pRenderTarget);
    SafeRelease(&m_pLightSlateGrayBrush);
    SafeRelease(&m_pCornflowerBlueBrush);

    free(m_cell1);
    free(m_cell2);
}

UINT DemoApp::CellIndex(UINT x, UINT y) {
    return (y * GridWidth) + x;
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
        m_hwndParent = CreateWindow(
            L"KabLife",
            L"KabLife",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            static_cast<UINT>(ceil((GridWidth * 10) + 18)),
            static_cast<UINT>(ceil((GridHeight * 10) + 10)),
            NULL,
            NULL,
            HINST_THISCOMPONENT,
            this
        );
        hr = m_hwndParent ? S_OK : E_FAIL;
        if (SUCCEEDED(hr))
        {
            m_hwndStartButton = CreateWindow(
                L"BUTTON", 
                L"Start", 
                WS_TABSTOP | WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 
                (GridWidth * 10) - 80, 
                2, 
                75, 
                25, 
                m_hwndParent, 
                NULL, 
                (HINSTANCE)GetWindowLongPtr(m_hwndParent, GWLP_HINSTANCE), 
                NULL
            );

            wcex.lpszClassName = L"RenderTarget";
            RegisterClassEx(&wcex);
            m_hwndRenderTarget = CreateWindow(
                L"RenderTarget",
                L"render",
                WS_CHILD | WS_VISIBLE,
                0,
                30,
                GridWidth * 10,
                GridHeight * 10 - 30,
                m_hwndParent,
                NULL,
                (HINSTANCE)GetWindowLongPtr(m_hwndParent, GWLP_HINSTANCE),
                NULL
            );

            ShowWindow(m_hwndParent, SW_SHOWNORMAL);
            UpdateWindow(m_hwndParent);
            InvalidateRect(m_hwndStartButton, NULL, FALSE);
        }
    }

    return hr;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    // Create a Direct2D factory.
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);

    if (SUCCEEDED(hr)) {
        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
        );
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pDWriteFactory->CreateTextFormat(
            L"Arial",                // Font family name.
            NULL,                       // Font collection (NULL sets it to use the system font collection).
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            20.0f,
            L"en-us",
            &m_pTextFormat
        );
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    if (!m_pRenderTarget)
    {
        RECT rc;
        GetClientRect(m_hwndRenderTarget, &rc);

        D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top
        );

        // Create a Direct2D render target.
        hr = m_pDirect2dFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwndRenderTarget, size),
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
        if (x > 0 && cell[CellIndex(x - 1, y - 1)]) result++;
        if (cell[((y - 1) * GridWidth) + x]) result++;
        if (x + 1 < GridWidth && cell[((y - 1) * GridWidth) + x + 1]) result++;
    }

    if (x > 0 && cell[(y * GridWidth) + x - 1]) result++;
    if (x + 1 < GridWidth && cell[(y * GridWidth) + x + 1]) result++;

    if (y + 1 < GridHeight) {
        if (x > 0 && cell[((y + 1) * GridWidth) + x - 1]) result++;
        if (y > 0 && cell[((y + 1) * GridWidth) + x]) result++;
        if (x + 1 < GridWidth && cell[((y + 1) * GridWidth) + x + 1]) result++;
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

    pDemoApp->m_iterationCount = 0;
    do {
        pDemoApp->m_iterationCount++; 
        if (pDemoApp->m_cell == &pDemoApp->m_cell1[0]) {
            oldGrid = &pDemoApp->m_cell1[0];
            newGrid = &pDemoApp->m_cell2[0];
        }
        else {
            oldGrid = &pDemoApp->m_cell2[0];
            newGrid = &pDemoApp->m_cell1[0];
        }

        for (int x = 0; x < pDemoApp->GridWidth; x++) {
            for (int y = 0; y < pDemoApp->GridHeight; y++) {
                neighbors = pDemoApp->CountNeighbors(x, y, oldGrid);
                BOOL alive = oldGrid[(y * pDemoApp->GridWidth) + x];

                if ((alive && (neighbors == 2 || neighbors == 3)) || (!alive && neighbors == 3)) {
                    newGrid[(y * pDemoApp->GridWidth) + x] = TRUE;
                }
                else {
                    newGrid[(y * pDemoApp->GridWidth) + x] = FALSE;
                }
            }
        }

        pDemoApp->m_cell = newGrid;
        tempGrid = oldGrid;
        oldGrid = newGrid;
        newGrid = tempGrid;

        InvalidateRect(pDemoApp->m_hwndParent, NULL, FALSE);
    }
    while (WaitForSingleObject(pDemoApp->m_hRunMutex, 75L) == WAIT_TIMEOUT);
}

void DemoApp::OnStartButton(DemoApp *pDemoApp) {
    pDemoApp->m_cell = pDemoApp->m_cell1;

    pDemoApp->m_hRunMutex = CreateMutexW(NULL, TRUE, NULL);

    for (int x = 0; x < pDemoApp->GridWidth; x++) {
        for (int y = 0; y < pDemoApp->GridHeight; y++) {
            if (rand() % 100 > 50) pDemoApp->m_cell[pDemoApp->CellIndex(x, y)] = TRUE;
            else pDemoApp->m_cell[pDemoApp->CellIndex(x, y)] = FALSE;
        }
    }

    _beginthread(DemoApp::ProcessProc, 0, pDemoApp);
}

HRESULT DemoApp::OnRender()
{
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();
    if (SUCCEEDED(hr))
    {
        wchar_t wszText[20];
        swprintf(wszText, 20, L"Iteration: %d", m_iterationCount);
        UINT32 cTextLength_ = (UINT32)wcslen(wszText);

        BOOL* cell = m_cell;

        m_pRenderTarget->BeginDraw();

        m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
        m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

        // Draw a grid background.
        int width = GridWidth * 10 + 2;
        int height = GridHeight * 10 + 2;

        // Draw grid first
        for (int x = 0; x <= GridWidth; x++) {
            m_pRenderTarget->DrawLine(
                D2D1::Point2F(static_cast<FLOAT>((x * 10) + 1), 0),
                D2D1::Point2F(static_cast<FLOAT>((x * 10) + 1), height),
                m_pLightSlateGrayBrush,
                0.5f
            );
        }

        for (int y = 0; y <= GridHeight; y++) {
            m_pRenderTarget->DrawLine(
                D2D1::Point2F(0.0f, static_cast<FLOAT>((y * 10) + 1)),
                D2D1::Point2F(width, static_cast<FLOAT>((y * 10) + 1)),
                m_pLightSlateGrayBrush,
                0.5f
            );
        }

        if (m_cell) {
            // Then draw cells
            for (int x = 0; x < GridWidth; x++) {
                for (int y = 0; y < GridHeight; y++) {
                    if (cell[CellIndex(x, y)]) {
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

            D2D1_RECT_F layoutRect = D2D1::RectF(
                10.0f,
                0.0f,
                300.0f,
                30.0f
            );

            m_pRenderTarget->DrawText(
                wszText,        // The string to render.
                cTextLength_,    // The string's length.
                m_pTextFormat,    // The text format.
                layoutRect,       // The region of the window where the text will be rendered.
                m_pCornflowerBlueBrush     // The brush used to draw the text.
            );
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

        result = 1;
    }
    else if (message == WM_COMMAND) {
          DemoApp* pDemoApp = reinterpret_cast<DemoApp*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        DemoApp::OnStartButton(pDemoApp);
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