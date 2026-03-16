#include "Game.h"
#include <iostream>

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


int main()
{
	exampleGame();
}

