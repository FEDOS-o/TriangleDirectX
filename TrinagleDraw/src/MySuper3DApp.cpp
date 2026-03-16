#include "Game.h"
#include <iostream>
#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

void exampleGame() {
	Game game(L"My3dApp", GetModuleHandle(nullptr), 800, 800);


	BackgroundGameComponent backComp(&game);
	game.components.push_back(&backComp);

	TriangleGameComponent rectComp(&game);
	game.components.push_back(&rectComp);


	game.Initialize();

	game.Run();

	std::cout << "Hello world!\n";
}

struct Rect {
	Vector2 position;
	Vector2 size;


	float Left() const {
		return position.x - size.x / 2; 
	}


	float Right() const { 
		return position.x + size.x / 2; 
	}


	float Top() const { 
		return position.y - size.y / 2; 
	}


	float Bottom() const { 
		return position.y + size.y / 2; 
	}

	bool Intersects(const Rect& other) const {
		return !(Right() < other.Left() ||
			Left() > other.Right() ||
			Bottom() < other.Top() ||
			Top() > other.Bottom());
	}
};

class PaddleGameComponent : public GameComponent {

private:
	Rect bounds;
	float speed;
	WPARAM upKey;
	WPARAM downKey;
	float minY;
	float maxY;

	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

public:
	PaddleGameComponent(Game* game, Vector2 startPos, WPARAM upKey, WPARAM downKey, float minY = -350.0f, float maxY = -350.0f) : 
		GameComponent(game),
		bounds{ startPos, Vector2(20.0f, 100.0f) },
		speed(500.0f),
		upKey(upKey),
		downKey(downKey),
		minY(minY),
		maxY(maxY) {}

	void Update(float deltaTime) override {
		if (GetAsyncKeyState(upKey) & 0x8000) {
			bounds.position.y -= speed * deltaTime;
		}

		if (GetAsyncKeyState(downKey) & 0x8000) {
			bounds.position.y += speed * deltaTime;
		}


		float halfHeight = bounds.size.y / 2;
		if (bounds.position.y - halfHeight < minY) {
			bounds.position.y = minY + halfHeight;
		}
		if (bounds.position.y + halfHeight > maxY) {
			bounds.position.y = maxY - halfHeight;
		}
	}

	Rect GetBounds() const {
		return bounds;
	}
};


class BallGameComponent : public GameComponent {
private:
	Rect bounds;
	Vector2 velocity;
	float speed;

	PaddleGameComponent* leftPaddle;
	PaddleGameComponent* rightPaddle;

	void PaddleCollision(PaddleGameComponent* paddle, int direction) {
		if (!paddle || !bounds.Intersects(paddle->GetBounds())) {
			return;
		}
		//!!!!
		velocity.x = direction * abs(velocity.x);  

		float hitPos = (bounds.position.y - paddle->GetBounds().position.y) /
			(paddle->GetBounds().size.y / 2);
		velocity.y = hitPos * 300.0f;
		
	}

public:
	BallGameComponent(Game* game, Vector2 startPos, PaddleGameComponent* leftPaddle, PaddleGameComponent* rightPaddle) :
		GameComponent(game),
		bounds{ startPos, Vector2(20.0f, 20.0f) },
		velocity(300.0f, 200.0f),
		speed(400.0f),
		leftPaddle(leftPaddle),
		rightPaddle(rightPaddle) {}

	void Update(float deltaTime) override {
		bounds.position.x += velocity.x * deltaTime;
		bounds.position.y += velocity.y * deltaTime;

		if (bounds.Top() < -400.0f || bounds.Bottom() > 400.0f) {
			velocity.y *= -1;
		}

		PaddleCollision(leftPaddle, 1);
		PaddleCollision(rightPaddle, -1);


		if (bounds.Right() > 450.0f) {
			std::cout << "Гол левому!\n" << std::endl;
			ResetBall(1);  // летит влево
		}
		if (bounds.Left() < -450.0f) {
			std::cout << "Гол правому!\n" << std::endl;
			ResetBall(-1);  // летит вправо
		}
	}

	void ResetBall(int direction) {
		bounds.position = Vector2(0, 0);
		velocity = Vector2(direction * speed, (rand() % 400 - 200.0f));
	}

	Rect GetBounds() const { return bounds; }
};


class WallComponent : public GameComponent {
private: 
	Rect bounds;

public:
	WallComponent(Game* game, Vector2 startPos) : GameComponent(game), bounds{ startPos, Vector2(900, 20) } {}
};



int main()
{
	exampleGame();
}

