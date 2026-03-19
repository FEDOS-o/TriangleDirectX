#include "Game.h"
#include "TriangleGameComponent.h"
#include "BackgroundGameComponent.h"
#include "RotatingTriangleGameComponent.h"
#include <iostream>

void exampleGame() {
    Game game(L"Orbiting Triangle", GetModuleHandle(nullptr), 800, 800);

    BackgroundGameComponent backComp(&game);
    game.components.push_back(&backComp);


    for (int i = 0; i < 10; i++) {
        RotatingTriangleComponent* rotatingTriangle = new RotatingTriangleComponent(&game, 0.0f, 0.0f, 0.3f, 1.0f, 0.314f * i);
        game.components.push_back(rotatingTriangle);
    }

    game.Initialize();
    game.Run();

    std::cout << "Game finished!\n";
}

int main() {
    exampleGame();
}

