#include "stdafx.h"
#include "PonyGameDirect2D.h"

#include "PonyGameString.h"

HRESULT PonyGame::InitializeWindow(const char* gameName, const int width, const int height)
{
	HRESULT hr;

	// Initialize device-independent resources, such as the Direct2D factory.
	hr = CreateDeviceIndependentResources();

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	// Register the window class.
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(LONG_PTR);
	wcex.hInstance = HINST_THISCOMPONENT;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wcex.lpszClassName = L"PonyGame";

	RegisterClassEx(&wcex);

	// Because the CreateWindow function takes its size in pixels,
	// obtain the system DPI and use it to scale the window size.
	FLOAT dpiX, dpiY;

	// The factory returns the current system DPI.
	// This is also the value it will use to create its own windows.
	direct2dFactory->GetDesktopDpi(&dpiX, &dpiY);

	// Create the window.
	hwnd = CreateWindow(
		L"PonyGame",
		StringToWString(std::string(gameName)).c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		static_cast<UINT>(ceil(width * dpiX / 96.f)),
		static_cast<UINT>(ceil(height * dpiY / 96.f)),
		NULL,
		NULL,
		HINST_THISCOMPONENT,
		(LPVOID)1
		);

	hr = hwnd ? S_OK : E_FAIL;

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);
	return hr;
}

HRESULT PonyGame::CreateDeviceIndependentResources()
{
	HRESULT hr = S_OK;

	// Create a Direct2D factory for creating render targets.
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &direct2dFactory);

	return hr;
}

HRESULT PonyGame::CreateDeviceResources()
{
	HRESULT hr = S_OK;

	if (renderTarget)
	{
		// Nothing to do.
		return hr;
	}

	RECT rc;
	GetClientRect(hwnd, &rc);

	D2D1_SIZE_U size = D2D1::SizeU(
		rc.right - rc.left,
		rc.bottom - rc.top
		);

	// Create a Direct2D render target for rendering to the window.
	hr = direct2dFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(hwnd, size),
		&renderTarget
		);

	return hr;
}

void PonyGame::DiscardDeviceResources()
{
	SafeRelease(&renderTarget);
}

LRESULT CALLBACK PonyGame::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (message == WM_CREATE)
	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;

		::SetWindowLongPtrW(
			hwnd,
			GWLP_USERDATA,
			PtrToUlong(pcs->lpCreateParams)
			);

		result = 1;
	}
	else
	{
		LONG_PTR window = static_cast<LONG_PTR>(
			::GetWindowLongPtrW(
				hwnd,
				GWLP_USERDATA
				));

		bool wasHandled = false;

		if (window)
		{
			switch (message)
			{
			case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);
				OnResize(width, height);
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
				OnRender();
				ValidateRect(hwnd, NULL);
			}
			result = 0;
			wasHandled = true;
			break;

			case WM_DESTROY:
			{
				PostQuitMessage(0);
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

HRESULT PonyGame::OnRender()
{
	HRESULT hr = S_OK;

	hr = CreateDeviceResources();

	if (!SUCCEEDED(hr))
	{
		return hr;
	}

	renderTarget->BeginDraw();
	{
		renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		renderTarget->Clear(clearColor);
	}
	hr = renderTarget->EndDraw();

	if (hr == D2DERR_RECREATE_TARGET)
	{
		hr = S_OK;
		DiscardDeviceResources();
	}

	return hr;
}

void PonyGame::OnResize(UINT width, UINT height)
{
	if (!renderTarget)
	{
		return;
	}

	// Note: This method can fail, but it's okay to ignore the
	// error here, because the error will be returned again
	// the next time EndDraw is called.
	renderTarget->Resize(D2D1::SizeU(width, height));
}


PONYGAMENATIVE_API bool Initialize(const char* gameName, const int width, const int height)
{
	using namespace PonyGame;

	if (!SUCCEEDED(CoInitialize(NULL)))
	{
		return false;
	}

	hwnd = NULL;
	direct2dFactory = NULL;
	renderTarget = NULL;
	clearColor = D2D1::ColorF::CornflowerBlue;

	return SUCCEEDED(InitializeWindow(gameName, width, height));
}

PONYGAMENATIVE_API void SetClearColor(float red, float green, float blue, float alpha)
{
	using namespace PonyGame;

	clearColor = D2D1::ColorF(red, green, blue, alpha);
}

PONYGAMENATIVE_API bool Render(void)
{
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
	{
		return false;
	}

	TranslateMessage(&msg);
	DispatchMessage(&msg);
	return true;
}

PONYGAMENATIVE_API void Uninitialize(void)
{
	using namespace PonyGame;

	SafeRelease(&direct2dFactory);
	SafeRelease(&renderTarget);

	CoUninitialize();
}