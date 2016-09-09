#pragma once

#include "d3dUtil.h"
#include "GameTimer.h"
#include <string>

class D3DApp
{
public:
	D3DApp(HINSTANCE hInstance);
	virtual ~D3DApp();

	HINSTANCE AppInst() const;
	HWND MainWnd() const;
	float AspectRatio() const;

	int Run();

	// Framework methods.  Derived client class overrides these methods to 
	// implement specific application requirements.

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnMouseDown(WPARAM btn_state, int x, int y) { };
	virtual void OnMouseUp(WPARAM btn_state, int x, int y) { };
	virtual void OnMouseMove(WPARAM btn_state, int x, int y) { };

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	void CalculateFrameStats();

protected:
	HINSTANCE app_instance_;
	HWND main_wnd_;
	bool is_paused_;
	bool is_minimized_;
	bool is_maxized_;
	bool is_resizing_;
	UINT x4_msaa_quality_;
	GameTimer timer_;

	ID3D11Device* d3d_device_;
	ID3D11DeviceContext* d3d_context_;
	IDXGISwapChain* swap_chain_;
	ID3D11Texture2D* depth_stencil_buffer_;
	ID3D11RenderTargetView* render_target_view_;
	ID3D11DepthStencilView* depth_stencil_view_;
	D3D11_VIEWPORT screen_viewport_;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring main_wnd_caption_;
	D3D_DRIVER_TYPE d3d_driver_type_;
	int client_width_;
	int client_height_;
	bool enable_4x_msaa_;
};