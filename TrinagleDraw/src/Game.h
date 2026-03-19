#pragma once

#include <windows.h>
#include <WinUser.h>
#include <wrl.h>
#include <iostream>
#include <d3d.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <chrono>
#include <cstdlib>
#include <vector>
#include "DisplayWin32.h"
#include "GameComponent.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

class GameComponent;



class Game {

private:
	HINSTANCE Instance;
	LPCWSTR Name;

	IDXGISwapChain* SwapChain;
	ID3D11Texture2D* BackBuffer;
	ID3D11UnorderedAccessView* RenderSRV;
	ID3D11Debug* DebugAnnotation;
	ID3D11RasterizerState* RasterizerState;

	std::chrono::time_point<std::chrono::steady_clock> PrevTime;
	std::chrono::time_point<std::chrono::steady_clock> StartTime;
	float TotalTime;

	bool ScreenResized;

public:

	ID3D11RenderTargetView* RenderView;
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	ID3D11DeviceContext* Context;
	DisplayWin32 Display;
	std::vector<GameComponent*> components;

public:

	Game(LPCWSTR applicationName, HINSTANCE hInstance, LONG screenWidth, LONG screenHeight) :
		Display(screenWidth, screenHeight, hInstance, applicationName),
		Instance(hInstance),
		Name(applicationName),
		TotalTime(0.0f),
		ScreenResized(false) {

		D3D_FEATURE_LEVEL featureLevel[] = { D3D_FEATURE_LEVEL_11_1 };

		DXGI_SWAP_CHAIN_DESC swapDesc = {};
		swapDesc.BufferCount = 2;
		swapDesc.BufferDesc.Width = screenWidth;
		swapDesc.BufferDesc.Height = screenHeight;
		swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapDesc.OutputWindow = Display.Window;
		swapDesc.Windowed = true;
		swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapDesc.SampleDesc.Count = 1;
		swapDesc.SampleDesc.Quality = 0;

		auto res = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG,
			featureLevel,
			1,
			D3D11_SDK_VERSION,
			&swapDesc,
			&SwapChain,
			&Device,
			nullptr,
			&Context);

		if (FAILED(res))
		{
			// Well, that was unexpected
		}

		Device->QueryInterface(__uuidof(ID3D11Debug), (void**)&DebugAnnotation);

		PrevTime = std::chrono::steady_clock::now();
		StartTime = PrevTime;
	}


	~Game() {
		DestroyResources();
	}


	HRESULT CreateBackBuffer() {
		if (RenderView) {
			RenderView->Release();
			RenderView = nullptr;
		}
		if (BackBuffer) {
			BackBuffer->Release();
			BackBuffer = nullptr;
		}

		auto res = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);	// __uuidof(ID3D11Texture2D)
		if (FAILED(res)) {
			Display.createMessageBox(L"Failed to get back buffer", L"Error", MB_OK);
			return res;
		}

		res = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderView);
		if (FAILED(res)) {
			Display.createMessageBox(L"Failed to create render target view", L"Error", MB_OK);
			return res;
		}
		return S_OK;
	}


	HRESULT Initialize() {
		auto res = CreateBackBuffer();
		if (FAILED(res)) {
			return res;
		}

		CD3D11_RASTERIZER_DESC rastDesc = {};
		rastDesc.CullMode = D3D11_CULL_NONE;
		rastDesc.FillMode = D3D11_FILL_SOLID;

		res = Device->CreateRasterizerState(&rastDesc, &RasterizerState);
		if (FAILED(res)) {
			Display.createMessageBox(L"Failed to create rasterizer state", L"Error", MB_OK);
			return res;
		}


		for (auto* component : components) {
			component->Initialize();
		}

		return S_OK;
	}


	void Update() {
		auto currentTime = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - PrevTime).count() / 1000000.0f;

		PrevTime = currentTime;

		TotalTime += deltaTime;

		for (auto* component : components) {
			component->Update(deltaTime);
		}

		UpdateInternal(deltaTime);
	}


	void UpdateInternal(float deltaTime) {

	}


	void PrepareFrame() {
		Context->ClearState();

		if (ScreenResized) {
			RestoreTargets();
			ScreenResized = false;
		}

		if (RasterizerState) {
			Context->RSSetState(RasterizerState);
		}

		D3D11_VIEWPORT viewport{};
		viewport.Width = 800.0f;
		viewport.Height = 800.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;

		Context->RSSetViewports(1, &viewport);
	}


	void PrepareResources() {

	}


	void RestoreTargets() {
		if (RenderView) {
			RenderView->Release();
			RenderView = nullptr;
		}
		if (BackBuffer) {
			BackBuffer->Release();
			BackBuffer = nullptr;
		}

		CreateBackBuffer();
	}


	void Draw() {
		float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		Context->ClearRenderTargetView(RenderView, clearColor);

		Context->OMSetRenderTargets(1, &RenderView, nullptr);

		for (auto* component : components) {
			component->Draw();
		}

		Context->OMSetRenderTargets(0, nullptr, nullptr);
	}


	void EndFrame() {
		SwapChain->Present(1, 0);
	}


	void MessageHandler(MSG& msg) {
		switch (msg.message) {
		case WM_KEYDOWN:
			if (msg.wParam == VK_ESCAPE) {
				Exit();
			}
			break;
		case WM_SIZE:
			ScreenResized = true;
			break;
		}
	}


	void Exit() {
		PostQuitMessage(0);
	}


	void DestroyResources() {
		for (auto* component : components) {
			component->DestroyResources();
		}
		if (RasterizerState) {
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
		if (RenderView) {
			RenderView->Release();
			RenderView = nullptr;
		}
		if (BackBuffer) {
			BackBuffer->Release();
			BackBuffer = nullptr;
		}
		if (RenderSRV) {
			RenderSRV->Release();
			RenderSRV = nullptr;
		}
		if (DebugAnnotation) {
			DebugAnnotation->Release();
			DebugAnnotation = nullptr;
		}
		if (Context) {
			Context->Release();
			Context = nullptr;
		}
		if (SwapChain) {
			SwapChain->Release();
			SwapChain = nullptr;
		}
	}


	void Run() {
		MSG msg = {};

		while (true) {
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				MessageHandler(msg);
			}

			if (msg.message == WM_QUIT) {
				break;
			}

			Update();

			PrepareFrame();
			PrepareResources();

			Draw();

			EndFrame();
		}
	}
};





