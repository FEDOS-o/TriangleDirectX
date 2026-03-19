#pragma once

#include "TriangleGameComponent.h"
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

class RotatingTriangleComponent : public TriangleGameComponent {
private:
	Vector2 center;     
	float radius;  
	float speed;
	float angle;

	ID3D11Buffer* constantBuffer = nullptr;

	struct ConstantBufferData {
		Matrix transformMatrix;
	};

public:
	RotatingTriangleComponent(Game* game, float centerX, float centerY, float radius, float speed, float angle)
		: TriangleGameComponent(game),
		center(centerX, centerY),
		radius(radius),
		speed(speed),
		angle(angle) {
	}

	~RotatingTriangleComponent() {
		if (constantBuffer) {
			constantBuffer->Release();
			constantBuffer = nullptr;
		}
	}

	void Initialize() override {
		TriangleGameComponent::Initialize();

		if (!game || !game->Device) return;

		D3D11_BUFFER_DESC constantBufDesc = {};
		constantBufDesc.Usage = D3D11_USAGE_DEFAULT;
		constantBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		constantBufDesc.CPUAccessFlags = 0;
		constantBufDesc.MiscFlags = 0;
		constantBufDesc.StructureByteStride = 0;
		constantBufDesc.ByteWidth = sizeof(ConstantBufferData);

		game->Device->CreateBuffer(&constantBufDesc, nullptr, &constantBuffer);

		Vector4 points[8] = {
			Vector4(0.0f, 0.15f, 0.5f, 1.0f),	Vector4(1.0f, 0.0f, 0.0f, 1.0f),
			Vector4(-0.13f, -0.075f, 0.5f, 1.0f),	Vector4(0.0f, 1.0f, 0.0f, 1.0f),
			Vector4(0.13f, -0.075f, 0.5f, 1.0f),	Vector4(0.0f, 0.0f, 1.0f, 1.0f),
			Vector4(0.0f, 0.0f, 0.5f, 1.0f),		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		};

		D3D11_BUFFER_DESC vertexBufDesc = {};
		vertexBufDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexBufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufDesc.CPUAccessFlags = 0;
		vertexBufDesc.MiscFlags = 0;
		vertexBufDesc.StructureByteStride = 0;
		vertexBufDesc.ByteWidth = sizeof(points);

		D3D11_SUBRESOURCE_DATA vertexData = {};
		vertexData.pSysMem = points;

		if (vb) vb->Release();
		game->Device->CreateBuffer(&vertexBufDesc, &vertexData, &vb);

		int indices[] = { 0, 1, 2 };

		D3D11_BUFFER_DESC indexBufDesc = {};
		indexBufDesc.Usage = D3D11_USAGE_DEFAULT;
		indexBufDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexBufDesc.CPUAccessFlags = 0;
		indexBufDesc.MiscFlags = 0;
		indexBufDesc.StructureByteStride = 0;
		indexBufDesc.ByteWidth = sizeof(indices);

		D3D11_SUBRESOURCE_DATA indexData = {};
		indexData.pSysMem = indices;

		if (ib) ib->Release();
		game->Device->CreateBuffer(&indexBufDesc, &indexData, &ib);

		if (strides) delete[] strides;
		if (offsets) delete[] offsets;

		strides = new UINT[1]{ 32 };
		offsets = new UINT[1]{ 0 };
	}

	void Update(float deltaTime) override {
		angle += speed * deltaTime;

		while (angle > DirectX::XM_2PI) {
			angle -= DirectX::XM_2PI;
		}
	}

	void Draw() override {
		if (!game || !game->Context || !game->RenderView || !vertexShader || !pixelShader || !vb || !ib || !constantBuffer) {
			return;
		}

		float x = center.x + radius * cos(angle);
		float y = center.y + radius * sin(angle);

		Matrix translationMatrix = Matrix::CreateTranslation(x, y, 0.0f);

		ConstantBufferData constantData;
		constantData.transformMatrix = translationMatrix.Transpose();

		game->Context->UpdateSubresource(constantBuffer, 0, nullptr, &constantData, 0, 0);

		game->Context->IASetInputLayout(layout);
		game->Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		game->Context->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
		game->Context->IASetVertexBuffers(0, 1, &vb, strides, offsets);

		game->Context->VSSetShader(vertexShader, nullptr, 0);
		game->Context->VSSetConstantBuffers(0, 1, &constantBuffer);
		game->Context->PSSetShader(pixelShader, nullptr, 0);

		game->Context->DrawIndexed(3, 0, 0);
	}

	void DestroyResources() override {
		if (constantBuffer) {
			constantBuffer->Release();
			constantBuffer = nullptr;
		}
		TriangleGameComponent::DestroyResources();
	}
};