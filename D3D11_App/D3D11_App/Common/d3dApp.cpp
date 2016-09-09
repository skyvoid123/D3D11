#include "d3dApp.h"
#include <Windowsx.h>
#include <sstream>

namespace
{
	// This is just used to forward Windows messages from a global window
	// procedure to our member function window procedure because we cannot
	// assign a member function to WNDCLASS::lpfnWndProc.
	D3DApp* g_d3dapp_ptr = nullptr;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.

	return g_d3dapp_ptr->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp::D3DApp(HINSTANCE hInstance)
	: app_instance_(hInstance)
	, main_wnd_(nullptr)
	, is_paused_(false)
	, is_minimized_(false)
	, is_maxized_(false)
	, is_resizing_(false)
	, x4_msaa_quality_(0)
	, timer_()
	, d3d_device_(nullptr)
	, d3d_context_(nullptr)
	, swap_chain_(nullptr)
	, depth_stencil_buffer_(nullptr)
	, render_target_view_(nullptr)
	, depth_stencil_view_(nullptr)
	, screen_viewport_()
	, main_wnd_caption_(L"D3D11 Application")
	, d3d_driver_type_(D3D_DRIVER_TYPE_HARDWARE)
	, client_width_(800)
	, client_height_(600)
	, enable_4x_msaa_(false)
{
	ZeroMemory(&screen_viewport_, sizeof(D3D11_VIEWPORT));
	// Get a pointer to the application object so we can forward 
	// Windows messages to the object's window procedure through
	// the global window procedure.
	g_d3dapp_ptr = this;
}

D3DApp::~D3DApp()
{
	ReleaseCOM(render_target_view_);
	ReleaseCOM(depth_stencil_view_);
	ReleaseCOM(swap_chain_);
	ReleaseCOM(depth_stencil_buffer_);

	if (d3d_context_)
	{
		d3d_context_->ClearState();
	}
	
	ReleaseCOM(d3d_context_);
	ReleaseCOM(d3d_device_);
}

HINSTANCE D3DApp::AppInst() const
{
	return app_instance_;
}

HWND D3DApp::MainWnd() const
{
	return main_wnd_;
}

float D3DApp::AspectRatio() const
{
	return (float)client_width_ / client_height_;
}

int D3DApp::Run()
{
	MSG msg = { 0 };
	timer_.Reset();
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer_.Tick();
			if (!is_paused_)
			{
				CalculateFrameStats();
				UpdateScene(timer_.DeltaTime());
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Init()
{
	if (!InitMainWindow())
	{
		return false;
	}

	if (!InitDirect3D())
	{
		return false;
	}

	return true;
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = app_instance_;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"D3DWndClassName";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT rect = { 0, 0, client_width_, client_height_ };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	main_wnd_ = CreateWindow(
		L"D3DWndClassName",
		main_wnd_caption_.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		0,
		0,
		app_instance_,
		0);

	if (!main_wnd_)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(main_wnd_, SW_SHOW);
	UpdateWindow(main_wnd_);

	return true;
}

bool D3DApp::InitDirect3D()
{
	// Create the device and device context.
	UINT  create_device_flags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level;
	HRESULT hr = D3D11CreateDevice(
		nullptr,
		d3d_driver_type_,
		0,
		create_device_flags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&d3d_device_,
		&feature_level,
		&d3d_context_
	);

	if (FAILED(hr))
	{
		MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if (feature_level != D3D_FEATURE_LEVEL_11_0)
	{
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.
	HR(d3d_device_->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &x4_msaa_quality_));
	assert(x4_msaa_quality_ > 0);

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = client_width_;
	sd.BufferDesc.Height = client_height_;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA? 
	if (enable_4x_msaa_)
	{
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = x4_msaa_quality_ - 1;
	}
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1;
	sd.OutputWindow = main_wnd_;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."
	IDXGIDevice* dxgi_device = nullptr;
	HR(d3d_device_->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi_device));

	IDXGIAdapter* dxgi_adapter = nullptr;
	HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgi_adapter));

	IDXGIFactory* dxgi_factory = nullptr;
	HR(dxgi_adapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgi_factory));

	HR(dxgi_factory->CreateSwapChain(d3d_device_, &sd, &swap_chain_));
	
	ReleaseCOM(dxgi_device);
	ReleaseCOM(dxgi_adapter);
	ReleaseCOM(dxgi_factory);

	// The remaining steps that need to be carried out for d3d creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.
	OnResize();

	return true;
}

void D3DApp::OnResize()
{
	assert(d3d_context_);
	assert(d3d_device_);
	assert(swap_chain_);

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
	ReleaseCOM(render_target_view_);
	ReleaseCOM(depth_stencil_view_);
	ReleaseCOM(depth_stencil_buffer_);

	// Resize the swap chain and recreate the render target view.
	HR(swap_chain_->ResizeBuffers(1, client_width_, client_height_, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ID3D11Texture2D* back_buffer;
	HR(swap_chain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer));
	HR(d3d_device_->CreateRenderTargetView(back_buffer, 0, &render_target_view_));
	ReleaseCOM(back_buffer);

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depth_stencil_desc;
	depth_stencil_desc.Width = client_width_;
	depth_stencil_desc.Height = client_height_;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (enable_4x_msaa_)
	{
		depth_stencil_desc.SampleDesc.Count = 4;
		depth_stencil_desc.SampleDesc.Quality = x4_msaa_quality_ - 1;
	}
	else
	{
		depth_stencil_desc.SampleDesc.Count = 1;
		depth_stencil_desc.SampleDesc.Quality = 0;
	}

	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;

	HR(d3d_device_->CreateTexture2D(&depth_stencil_desc, 0, &depth_stencil_buffer_));
	HR(d3d_device_->CreateDepthStencilView(depth_stencil_buffer_, 0, &depth_stencil_view_));

	// Bind the render target view and depth/stencil view to the pipeline.
	d3d_context_->OMSetRenderTargets(1, &render_target_view_, depth_stencil_view_);

	// Set the viewport transform.
	screen_viewport_.TopLeftX = 0;
	screen_viewport_.TopLeftY = 0;
	screen_viewport_.Width = (float)client_width_;
	screen_viewport_.Height = (float)client_height_;
	screen_viewport_.MinDepth = 0.0f;
	screen_viewport_.MaxDepth = 1.0f;

	d3d_context_->RSSetViewports(1, &screen_viewport_);
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frame_count = 0;
	static float time_elapsed = 0.0f;

	++frame_count;

	// Compute averages over one second period.
	if ((timer_.TotalTime() - time_elapsed) >= 1.0f)
	{
		float fps = (float)frame_count;
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);
		outs << main_wnd_caption_ << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L"(ms)";
		SetWindowText(main_wnd_, outs.str().c_str());

		// Reset for next average.
		frame_count = 0;
		time_elapsed += 1.0f;
	}
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			is_paused_ = true;
			timer_.Stop();
		}
		else
		{
			is_paused_ = false;
			timer_.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		client_width_ = LOWORD(lParam);
		client_height_ = HIWORD(lParam);
		if (d3d_device_)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				is_paused_ = true;
				is_minimized_ = true;
				is_maxized_ = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				is_paused_ = false;
				is_minimized_ = false;
				is_maxized_ = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (is_minimized_)
				{
					is_paused_ = false;
					is_minimized_ = false;
					OnResize();
				}
				else if (is_maxized_)
				{
					is_paused_ = false;
					is_maxized_ = false;
					OnResize();
				}
				else if (is_resizing_)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else
				{
					// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
					OnResize();
				}
			}

		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		is_paused_ = true;
		is_resizing_ = true;
		timer_.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		is_paused_ = false;
		is_resizing_ = false;
		timer_.Start();
		OnResize();
		return 0;

	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	default:
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}