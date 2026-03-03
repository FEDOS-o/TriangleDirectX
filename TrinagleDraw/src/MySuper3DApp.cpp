// MySuper3DApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

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

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
	case WM_KEYDOWN:
	{
		// If a key is pressed send it to the input object so it can record that state.
		std::cout << "Key: " << static_cast<unsigned int>(wparam) << std::endl;

		if (static_cast<unsigned int>(wparam) == 27) PostQuitMessage(0);
		return 0;
	}
	default:
	{
		return DefWindowProc(hwnd, umessage, wparam, lparam);
	}
	}
}


class DisplayWin32 {
private:
	LONG clientHeight, clientWidth;
	HINSTANCE hInstance;
	HWND hWnd;
	WNDCLASSEX wc;


public:
	DisplayWin32(LONG clientWidth, LONG clientHeight, HINSTANCE hInstance, LPCWSTR applicationName) :
		clientHeight(clientHeight),
		clientWidth(clientWidth),
		hInstance(hInstance) {
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
		wc.hIconSm = wc.hIcon;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = applicationName;
		wc.cbSize = sizeof(WNDCLASSEX);

		// Register the window class.
		RegisterClassEx(&wc);


		auto screenWidth = 800;
		auto screenHeight = 800;

		RECT windowRect = { 0, 0, static_cast<LONG>(screenWidth), static_cast<LONG>(screenHeight) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		auto dwStyle = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_THICKFRAME;

		auto posX = (GetSystemMetrics(SM_CXSCREEN) - screenWidth) / 2;
		auto posY = (GetSystemMetrics(SM_CYSCREEN) - screenHeight) / 2;

		hWnd = CreateWindowEx(WS_EX_APPWINDOW, applicationName, applicationName,
			dwStyle,
			posX, posY,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr, nullptr, hInstance, nullptr);

		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);

		ShowCursor(true);
	}


	HWND getHWnd() {
		return hWnd;
	}

	void createMessageBox(LPCWSTR text, LPCWSTR caption, UINT type) {
		MessageBox(hWnd, text, caption, type);
	}

	void setWindowText(LPCWSTR text) {
		SetWindowText(hWnd, text);
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

public:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	ID3D11DeviceContext* context;
	IDXGISwapChain* swapChain;
	DisplayWin32 display;
	std::vector<GameComponent*> components;

public:

	Game(LPCWSTR applicationName, HINSTANCE hInstance, LONG screenWidth, LONG screenHeight)
		: display(screenWidth, screenHeight, hInstance, applicationName) {

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
		swapDesc.OutputWindow = display.getHWnd();
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
			&swapChain,
			&device,
			nullptr,
			&context);

		if (FAILED(res))
		{
			// Well, that was unexpected
		}

	}



};

class TriangleGameComponent : GameComponent {
public:
	ID3D11Texture2D* backTex;
	ID3D11RenderTargetView* rtv;
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
		auto res = game->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backTex);	// __uuidof(ID3D11Texture2D)
		if (FAILED(res)) {
			game->display.createMessageBox(L"Failed to get back buffer", L"Error", MB_OK);
			return;
		}
		res = game->device->CreateRenderTargetView(backTex, nullptr, &rtv);
		if (FAILED(res)) {
			game->display.createMessageBox(L"Failed to create render target view", L"Error", MB_OK);
			return;
		}


		res = D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl",
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
				game->display.createMessageBox(L"MyVeryFirstShader.hlsl", L"Missing Shader File", MB_OK);
				//MessageBox(hWnd, L"MyVeryFirstShader.hlsl", L"Missing Shader File", MB_OK);
			}

			std::exit(0);
		}

		D3D_SHADER_MACRO Shader_Macros[] = { "TEST", "1", "TCOLOR", "float4(0.0f, 1.0f, 0.0f, 1.0f)", nullptr, nullptr };

		res = D3DCompileFromFile(L"./Shaders/MyVeryFirstShader.hlsl", Shader_Macros /*macros*/, nullptr /*include*/, "PSMain", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelBC, &errorPixelCode);


		game->device->CreateVertexShader(
			vertexBC->GetBufferPointer(),
			vertexBC->GetBufferSize(),
			nullptr, &vertexShader);

		game->device->CreatePixelShader(
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

		game->device->CreateInputLayout(
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

		game->device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);


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

		game->device->CreateBuffer(&indexBufDesc, &indexData, &ib);

		strides = new UINT[1]{ 32 };
		offsets = new UINT[1]{ 0 };
	}

	void Update(float deltaTime) override {
		
	}


	void Draw() {
		if (!game || !game->context) {
			return;
		}


		game->context->IASetInputLayout(layout);
		game->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		game->context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
		game->context->IASetVertexBuffers(0, 1, &vb, strides, offsets);
		game->context->VSSetShader(vertexShader, nullptr, 0);
		game->context->PSSetShader(pixelShader, nullptr, 0);
	}


	void DestroyResources() override {
		if (rtv) { 
			rtv->Release(); 
			rtv = nullptr; 
		}
		if (backTex) { 
			backTex->Release(); 
			backTex = nullptr; 
		}
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






int main()
{
	Game game(L"My3dApp", GetModuleHandle(nullptr), 800, 800);

	TriangleGameComponent comp(&game);
	comp.Initialize();


	CD3D11_RASTERIZER_DESC rastDesc = {};
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.FillMode = D3D11_FILL_SOLID;

	ID3D11RasterizerState* rastState;
	auto res = game.device->CreateRasterizerState(&rastDesc, &rastState);

	game.context->RSSetState(rastState);

	std::chrono::time_point<std::chrono::steady_clock> PrevTime = std::chrono::steady_clock::now();
	float totalTime = 0;
	unsigned int frameCount = 0;

	MSG msg = {};
	bool isExitRequested = false;
	while (!isExitRequested) {
		// Handle the windows messages.
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT) {
			isExitRequested = true;
		}

		game.context->ClearState();

		game.context->RSSetState(rastState);

		D3D11_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(800);
		viewport.Height = static_cast<float>(800);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1.0f;

		game.context->RSSetViewports(1, &viewport);

		comp.Draw();

		auto	curTime = std::chrono::steady_clock::now();
		float	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - PrevTime).count() / 1000000.0f;
		PrevTime = curTime;

		totalTime += deltaTime;
		frameCount++;

		if (totalTime > 1.0f) {
			float fps = frameCount / totalTime;

			totalTime -= 1.0f;

			WCHAR text[256];
			swprintf_s(text, TEXT("FPS: %f"), fps);
			game.display.setWindowText(text);
			//SetWindowText(hWnd, text);

			frameCount = 0;
		}

		game.context->OMSetRenderTargets(1, &comp.rtv, nullptr);

		float color[] = { totalTime, 0.1f, 0.1f, 1.0f };
		game.context->ClearRenderTargetView(comp.rtv, color);

		game.context->DrawIndexed(6, 0, 0);

		game.context->OMSetRenderTargets(0, nullptr, nullptr);

		game.swapChain->Present(1, /*DXGI_PRESENT_DO_NOT_WAIT*/ 0);
	}

	std::cout << "Hello World!\n";
}

