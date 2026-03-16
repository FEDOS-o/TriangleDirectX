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


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")


class DisplayWin32 {
private:
	LONG ClientHeight, ClientWidth;

	WNDCLASSEX Wc;

public:
	HWND Window;

public:
	DisplayWin32(LONG clientWidth, LONG clientHeight, HINSTANCE instance, LPCWSTR applicationName) :
		ClientHeight(clientHeight),
		ClientWidth(clientWidth) {
		Wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		Wc.lpfnWndProc = WndProc;
		Wc.cbClsExtra = 0;
		Wc.cbWndExtra = 0;
		Wc.hInstance = instance;
		Wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		Wc.hIconSm = Wc.hIcon;
		Wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		Wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		Wc.lpszMenuName = nullptr;
		Wc.lpszClassName = applicationName;
		Wc.cbSize = sizeof(WNDCLASSEX);

		// Register the window class.
		RegisterClassEx(&Wc);


		RECT windowRect = { 0, 0, static_cast<LONG>(ClientWidth), static_cast<LONG>(ClientHeight) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

		auto posX = (GetSystemMetrics(SM_CXSCREEN) - ClientWidth) / 2;
		auto posY = (GetSystemMetrics(SM_CYSCREEN) - ClientHeight) / 2;

		Window = CreateWindowEx(
			WS_EX_APPWINDOW,
			applicationName,
			applicationName,
			dwStyle,
			posX, posY,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			instance,
			nullptr);

		ShowWindow(Window, SW_SHOW);
		SetForegroundWindow(Window);
		SetFocus(Window);

		ShowCursor(true);
	}

	void createMessageBox(LPCWSTR text, LPCWSTR caption, UINT type) {
		MessageBox(Window, text, caption, type);
	}

	void setWindowText(LPCWSTR text) {
		SetWindowText(Window, text);
	}

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
	{
		switch (umessage)
		{
		case WM_KEYDOWN:
		{
			// If a key is pressed send it to the input object so it can record that state.
			std::cout << "Key: " << static_cast<unsigned int>(wparam) << std::endl;
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
		}
	}
};


class Game;


class GameComponent {
protected:
	Game* game;
public:
	GameComponent(Game* game) : game(game) {}

	virtual void Initialize() {};
	virtual void Update(float deltaTime) {};
	virtual void Draw() {};
	virtual void DestroyResources() {};
	virtual void Reload() {}
};


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


class TriangleGameComponent : public GameComponent {
public:
	ID3DBlob* vertexBC = nullptr;
	ID3DBlob* errorVertexCode = nullptr;
	ID3DBlob* pixelBC;
	ID3DBlob* errorPixelCode;
	ID3D11VertexShader* vertexShader;
	ID3D11PixelShader* pixelShader;
	ID3D11InputLayout* layout;
	ID3D11Buffer* vb;
	ID3D11Buffer* ib;
	UINT* strides;
	UINT* offsets;


	~TriangleGameComponent() {
		DestroyResources();
	}


	TriangleGameComponent(Game* game) : GameComponent{ game } {}


	void Initialize() override {
		auto res = D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl",
			nullptr /*macros*/,
			nullptr /*include*/,
			"VSMain",
			"vs_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0,
			&vertexBC,
			&errorVertexCode);

		if (FAILED(res)) {
			// If the shader failed to compile it should have written something to the error message.
			if (errorVertexCode) {
				char* compileErrors = (char*)(errorVertexCode->GetBufferPointer());

				std::cout << compileErrors << std::endl;
			}
			// If there was  nothing in the error message then it simply could not find the shader file itself.
			else
			{
				unsigned int mask = 1 << ((sizeof(int) << 3) - 1);
				while (mask) {
					printf("%d", (res & mask ? 1 : 0));
					mask >>= 1;
				}
				game->Display.createMessageBox(L"MyVeryFirstShader.hlsl", L"Missing Shader File", MB_OK);
				//MessageBox(hWnd, L"MyVeryFirstShader.hlsl", L"Missing Shader File", MB_OK);
			}

			std::exit(0);
		}

		D3D_SHADER_MACRO Shader_Macros[] = { "TEST", "1", "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr };

		res = D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl",
			Shader_Macros /*macros*/,
			nullptr /*include*/,
			"PSMain",
			"ps_5_0",
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
			0,
			&pixelBC,
			&errorPixelCode);


		game->Device->CreateVertexShader(
			vertexBC->GetBufferPointer(),
			vertexBC->GetBufferSize(),
			nullptr, &vertexShader);

		game->Device->CreatePixelShader(
			pixelBC->GetBufferPointer(),
			pixelBC->GetBufferSize(),
			nullptr, &pixelShader);

		D3D11_INPUT_ELEMENT_DESC inputElements[] = {
		D3D11_INPUT_ELEMENT_DESC {
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			0,
			D3D11_INPUT_PER_VERTEX_DATA,
			0},
		D3D11_INPUT_ELEMENT_DESC {
			"COLOR",
			0,
			DXGI_FORMAT_R32G32B32A32_FLOAT,
			0,
			D3D11_APPEND_ALIGNED_ELEMENT,
			D3D11_INPUT_PER_VERTEX_DATA,
			0}
		};

		game->Device->CreateInputLayout(
			inputElements,
			2,
			vertexBC->GetBufferPointer(),
			vertexBC->GetBufferSize(),
			&layout);

		DirectX::XMFLOAT4 points[8] = {
			DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
			DirectX::XMFLOAT4(-0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
			DirectX::XMFLOAT4(0.5f, -0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
			DirectX::XMFLOAT4(-0.5f, 0.5f, 0.5f, 1.0f),	DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		};


		D3D11_BUFFER_DESC vertexBufDesc = {};
		vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufDesc.CPUAccessFlags = 0;
		vertexBufDesc.MiscFlags = 0;
		vertexBufDesc.StructureByteStride = 0;
		vertexBufDesc.ByteWidth = sizeof(DirectX::XMFLOAT4) * std::size(points);

		D3D11_SUBRESOURCE_DATA vertexData = {};
		vertexData.pSysMem = points;
		vertexData.SysMemPitch = 0;
		vertexData.SysMemSlicePitch = 0;

		game->Device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);


		int indeces[] = { 0,1,2, 1,0,3 };
		D3D11_BUFFER_DESC indexBufDesc = {};
		indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufDesc.CPUAccessFlags = 0;
		indexBufDesc.MiscFlags = 0;
		indexBufDesc.StructureByteStride = 0;
		indexBufDesc.ByteWidth = sizeof(int) * std::size(indeces);

		D3D11_SUBRESOURCE_DATA indexData = {};
		indexData.pSysMem = indeces;
		indexData.SysMemPitch = 0;
		indexData.SysMemSlicePitch = 0;

		game->Device->CreateBuffer(&indexBufDesc, &indexData, &ib);

		strides = new UINT[1]{ 32 };
		offsets = new UINT[1]{ 0 };
	}


	void Update(float deltaTime) override {

	}


	void Draw() {
		if (!game || !game->Context) {
			return;
		}


		game->Context->IASetInputLayout(layout);
		game->Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		game->Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
		game->Context->IASetVertexBuffers(0, 1, &vb, strides, offsets);
		game->Context->VSSetShader(vertexShader, nullptr, 0);
		game->Context->PSSetShader(pixelShader, nullptr, 0);

		game->Context->DrawIndexed(6, 0, 0);
	}


	void DestroyResources() override {
		if (layout) {
			layout->Release();
			layout = nullptr;
		}
		if (vb) {
			vb->Release();
			vb = nullptr;
		}
		if (ib) {
			ib->Release();
			ib = nullptr;
		}
		if (vertexShader) {
			vertexShader->Release();
			vertexShader = nullptr;
		}
		if (pixelShader) {
			pixelShader->Release();
			pixelShader = nullptr;
		}
		if (vertexBC) {
			vertexBC->Release();
			vertexBC = nullptr;
		}
		if (pixelBC) {
			pixelBC->Release();
			pixelBC = nullptr;
		}
		if (errorVertexCode) {
			errorVertexCode->Release();
			errorVertexCode = nullptr;
		}
		if (errorPixelCode) {
			errorPixelCode->Release();
			errorPixelCode = nullptr;
		}
		if (strides) {
			delete[] strides;
			strides = nullptr;
		}
		if (offsets) {
			delete[] offsets;
			offsets = nullptr;
		}
	}
};


class BackgroundGameComponent : public GameComponent {
private:
	float TotalTime;

public:
	BackgroundGameComponent(Game* game) : GameComponent(game), TotalTime(0.0f) {}

	void Update(float deltaTime) override {
		TotalTime += deltaTime;
		while (TotalTime > 1.0f) {
			TotalTime -= 1.0f;
		}
	}

	void Draw() override {
		if (!game || !game->Context || !game->RenderView) {
			return;
		}

		float clearColor[] = { TotalTime, 0.1f, 0.1f, 1.0f };
		game->Context->ClearRenderTargetView(game->RenderView, clearColor);
	}
};



