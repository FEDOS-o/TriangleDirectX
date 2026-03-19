#pragma once
#include "GameComponent.h"
#include "Game.h"


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


	TriangleGameComponent(Game* game) : GameComponent(game) {}


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

