#include <Windows.h>
#include <time.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <texture.h>
#include <gameObjects.h>

typedef enum { ROLL, MOVE } GamePhase;
#define tilesSize 48

/* Variables */
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int BOARD_WIDTH = 400;
const int BOARD_HEIGHT = 400;
const SDL_Color RED = { 0xEE, 0x11, 0x11, 0xFF };
const SDL_Color GREEN = { 0x00, 0xAA, 0x22, 0xFF };
const SDL_Color BLUE = { 0x00, 0x44, 0xFF, 0xFF };
const SDL_Color YELLOW = { 0xFF, 0xDD, 0x00, 0xFF };
const SDL_Color BACKGROUND_WHITE = { 0xFA, 0xFA, 0xF0, 0xFF };
const int playerWidth = 15;
const int playerHeight = 23;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font;

void(*renderHandler)();
int(*eventHandler)(SDL_Event*);
void(*unloadHandler)();

/* Funcion declarations */
int init();
void close();

int loadMainMenu();
void renderMainMenu();
int handleMainMenuEvent(SDL_Event* e);
void unloadMainMenu();

int loadGame();
void renderGame();
int handleGameEvent(SDL_Event* e);
void unloadGame();
void setGamePhase(GamePhase p);

/* Function definitions */
int init()
{
	//Initialization flag
	int success = 1;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = 0;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		window = SDL_CreateWindow("Fia", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = 0;
		}
		else
		{
			//Create vsynced renderer for window
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (renderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = 0;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = 0;
				}
				//Initialize SDL_ttf
				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = 0;
				}
			}
		}
	}

	return success;
}

void close()
{
	unloadHandler();

	TTF_CloseFont(font);
	font = NULL;

	//Destroy window	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;

	//Quit SDL subsystems
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		/* Load global font */
		font = TTF_OpenFont("font.ttf", 28);
		if (font == NULL)
		{
			printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
		}
		else if (!loadMainMenu())
		{
			printf("Failed to load main menu!\n");
		}
		else
		{
			srand(time(NULL));
			SDL_Event e;

			// Game loop
			int quit = 0;
			while (!quit)
			{
				//The rerender text flag
				int renderText = 0;

				//Handle events on queue
				while (SDL_PollEvent(&e) != 0)
				{
					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = 1;
					}
					else
					{
						if (!eventHandler(&e))
						{
							quit = 1;
							printf("Failed to handle event!\n");
						}
					}
				}

				if (!quit)
				{
					//Clear screen
					SDL_SetRenderDrawColor(renderer, BACKGROUND_WHITE.r, BACKGROUND_WHITE.g, BACKGROUND_WHITE.b, BACKGROUND_WHITE.a);
					SDL_RenderClear(renderer);

					renderHandler();

					//Update screen
					SDL_RenderPresent(renderer);
				}
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}

/***** MAIN MENU *****/
Texture* background;
int loadMainMenu()
{
	int success = 1;

	background = (Texture*)malloc(sizeof(Texture));
	*background = (Texture){ .height = 0, .width = 0, .texture = NULL };
	if (!loadTextureFromFile(background, renderer, "background.png"))
	{
		printf("Failed to load texture image!\n");
		success = 0;
	}

	renderHandler = &renderMainMenu;
	eventHandler = &handleMainMenuEvent;
	unloadHandler = &unloadMainMenu;

	return success;
}

void unloadMainMenu()
{
	freeTexture(background);
	free(background);
}

void renderMainMenu()
{
	renderTexture(background, renderer, 0, 0, NULL, 0, NULL, SDL_FLIP_NONE);
}

int handleMainMenuEvent(SDL_Event* e)
{
	int success = 1;

	if (e->type == SDL_KEYDOWN)
	{
		if (e->key.keysym.sym == SDLK_RETURN)
		{
			unloadHandler();
			if (!loadGame()) {
				printf("Failed to load game!\n");
				success = 0;
			}
		}
	}

	return success;
}

/***** GAME *****/
Texture* gameMsgTexture;
Texture* playersSprite;
Die* die;
Tile tiles[tilesSize];
Team teams[4];
int turn = 0;
GamePhase phase = ROLL;
int selectedUnitIndex;
int pauseInput = 0;

// TODO: a list of animations that are worked through during game render
// TODO: pointer
DieAnimation dieAnimation;
int loadGame()
{
	int success = 1;

	playersSprite = (Texture*)malloc(sizeof(Texture));
	*playersSprite = (Texture){ .height = 0, .width = 0, .texture = NULL };
	if (!loadTextureFromFile(playersSprite, renderer, "players.png"))
	{
		printf("Failed to load texture image!\n");
		success = 0;
	}

	die = (Die*)malloc(sizeof(Die));
	*die = (Die) { .sides = 6, .currentValue = 0 };
	die->clip = (SDL_Rect){ .w = 46, .h = 46, .x = 0, .y = 0 };
	if (!loadTextureFromFile(&(die->texture), renderer, "die.png"))
	{
		printf("Failed to load texture image!\n");
		success = 0;
	}

	dieAnimation = (DieAnimation){ .remainingFrames = -1, .die = NULL };

	int radiusSmall = 46;
	int radiusBig = 100;
	int spacing = 13;

	int boardLeft = (SCREEN_WIDTH - BOARD_WIDTH) / 2;
	int boardTop = (SCREEN_HEIGHT - BOARD_HEIGHT) / 2;

	/* Spawn */
	tiles[37] = (Tile){ .nextTile = &tiles[0], .posX = boardLeft, .posY = boardTop + BOARD_HEIGHT - radiusSmall - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[37].color = RED;
	tiles[38] = (Tile){ .nextTile = &tiles[0], .posX = boardLeft + radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[38].color = RED;
	tiles[32] = (Tile){ .nextTile = &tiles[0], .posX = boardLeft, .posY = boardTop + BOARD_HEIGHT - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[32].color = RED;
	tiles[36] = (Tile){ .nextTile = &tiles[0], .posX = boardLeft + radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[36].color = RED;

	tiles[33] = (Tile){ .nextTile = &tiles[6], .posX = boardLeft, .posY = boardTop, .radius = radiusSmall, .type = SPAWN };
	tiles[33].color = GREEN;
	tiles[39] = (Tile){ .nextTile = &tiles[6], .posX = boardLeft + radiusSmall, .posY = boardTop, .radius = radiusSmall, .type = SPAWN };
	tiles[39].color = GREEN;
	tiles[40] = (Tile){ .nextTile = &tiles[6], .posX = boardLeft, .posY = boardTop + radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[40].color = GREEN;
	tiles[41] = (Tile){ .nextTile = &tiles[6], .posX = boardLeft + radiusSmall, .posY = boardTop + radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[41].color = GREEN;

	tiles[34] = (Tile){ .nextTile = &tiles[12], .posX = boardLeft + BOARD_WIDTH - radiusSmall, .posY = boardTop, .radius = radiusSmall, .type = SPAWN };
	tiles[34].color = BLUE;
	tiles[42] = (Tile){ .nextTile = &tiles[12], .posX = boardLeft + BOARD_WIDTH - radiusSmall, .posY = boardTop + radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[42].color = BLUE;
	tiles[43] = (Tile){ .nextTile = &tiles[12], .posX = boardLeft + BOARD_WIDTH - radiusSmall - radiusSmall, .posY = boardTop, .radius = radiusSmall, .type = SPAWN };
	tiles[43].color = BLUE;
	tiles[44] = (Tile){ .nextTile = &tiles[12], .posX = boardLeft + BOARD_WIDTH - radiusSmall - radiusSmall, .posY = boardTop + radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[44].color = BLUE;

	tiles[35] = (Tile) { .nextTile = &tiles[18], .posX = boardLeft + BOARD_WIDTH - radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[35].color = YELLOW;
	tiles[45] = (Tile){ .nextTile = &tiles[18], .posX = boardLeft + BOARD_WIDTH - radiusSmall - radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[45].color = YELLOW;
	tiles[46] = (Tile){ .nextTile = &tiles[18], .posX = boardLeft + BOARD_WIDTH - radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[46].color = YELLOW;
	tiles[47] = (Tile){ .nextTile = &tiles[18], .posX = boardLeft + BOARD_WIDTH - radiusSmall - radiusSmall, .posY = boardTop + BOARD_HEIGHT - radiusSmall - radiusSmall, .radius = radiusSmall, .type = SPAWN };
	tiles[47].color = YELLOW;

	/* Outer ring */
	tiles[0] = (Tile){ .nextTile = &tiles[1], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 6 * radiusSmall + 6 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[0].color = RED;
	tiles[1] = (Tile){ .nextTile = &tiles[2], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 5 * radiusSmall + 5 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[1].color = GREEN;
	tiles[2] = (Tile){ .nextTile = &tiles[3], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[2].color = BLUE;
	tiles[3] = (Tile){ .nextTile = &tiles[4], .posX = boardLeft + 1 * radiusSmall + 1 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[3].color = YELLOW;
	tiles[4] = (Tile){ .nextTile = &tiles[5], .posX = boardLeft, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[4].color = RED;
	tiles[5] = (Tile){ .nextTile = &tiles[6], .posX = boardLeft, .posY = boardTop + 3 * radiusSmall + 3 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[5].color = GREEN;
	tiles[6] = (Tile){ .nextTile = &tiles[7], .posX = boardLeft, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[6].color = GREEN;
	tiles[7] = (Tile){ .nextTile = &tiles[8], .posX = boardLeft + 1 * radiusSmall + 1 * spacing, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[7].color = BLUE;
	tiles[8] = (Tile){ .nextTile = &tiles[9], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[8].color = YELLOW;
	tiles[9] = (Tile){ .nextTile = &tiles[10], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 1 * radiusSmall + 1 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[9].color = RED;
	tiles[10] = (Tile){ .nextTile = &tiles[11], .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 0 * radiusSmall + 0 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[10].color = GREEN;
	tiles[11] = (Tile){ .nextTile = &tiles[12], .posX = boardLeft + 3 * radiusSmall + 3 * spacing, .posY = boardTop + 0 * radiusSmall + 0 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[11].color = BLUE;
	tiles[12] = (Tile){ .nextTile = &tiles[13], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 0 * radiusSmall + 0 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[12].color = BLUE;
	tiles[13] = (Tile){ .nextTile = &tiles[14], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 1 * radiusSmall + 1 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[13].color = YELLOW;
	tiles[14] = (Tile){ .nextTile = &tiles[15], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[14].color = RED;
	tiles[15] = (Tile){ .nextTile = &tiles[16], .posX = boardLeft + 5 * radiusSmall + 5 * spacing, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[15].color = GREEN;
	tiles[16] = (Tile){ .nextTile = &tiles[17], .posX = boardLeft + 6 * radiusSmall + 6 * spacing, .posY = boardTop + 2 * radiusSmall + 2 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[16].color = BLUE;
	tiles[17] = (Tile){ .nextTile = &tiles[18], .posX = boardLeft + 6 * radiusSmall + 6 * spacing, .posY = boardTop + 3 * radiusSmall + 3 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[17].color = YELLOW;
	tiles[18] = (Tile){ .nextTile = &tiles[19], .posX = boardLeft + 6 * radiusSmall + 6 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[18].color = YELLOW;
	tiles[19] = (Tile){ .nextTile = &tiles[20], .posX = boardLeft + 5 * radiusSmall + 5 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[19].color = RED;
	tiles[20] = (Tile){ .nextTile = &tiles[21], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[20].color = GREEN;
	tiles[21] = (Tile){ .nextTile = &tiles[22], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 5 * radiusSmall + 5 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[21].color = BLUE;
	tiles[22] = (Tile){ .nextTile = &tiles[23], .posX = boardLeft + 4 * radiusSmall + 4 * spacing, .posY = boardTop + 6 * radiusSmall + 6 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[22].color = YELLOW;
	tiles[23] = (Tile){ .nextTile = &tiles[0], .posX = boardLeft + 3 * radiusSmall + 3 * spacing, .posY = boardTop + 6 * radiusSmall + 6 * spacing, .radius = radiusSmall, .type = RING, .unit = NULL };
	tiles[23].color = RED;

	/* Finish tiles */
	tiles[24] = (Tile){ .nextTile = &tiles[25], .posX = boardLeft + 3 * radiusSmall + 3 * spacing, .posY = boardTop + 5 * radiusSmall + 5 * spacing, .radius = radiusSmall, .unit = NULL };
	tiles[24].color = RED;

	tiles[25] = (Tile){ .nextTile = NULL, .posX = boardLeft + 3 * radiusSmall + 3 * spacing, .posY = boardTop + 4 * radiusSmall + 4 * spacing, .radius = radiusSmall, .unit = NULL };
	tiles[25].color = RED;

	tiles[26] = (Tile){ .nextTile = &tiles[27], .posX = boardLeft + 1 * radiusSmall + 1 * spacing, .posY = boardTop + 3 * radiusSmall + 3 * spacing, .radius = radiusSmall, .unit = NULL };
	tiles[26].color = GREEN;

	tiles[27] = (Tile){ .nextTile = NULL, .posX = boardLeft + 2 * radiusSmall + 2 * spacing, .posY = boardTop + 3 * radiusSmall + 3 * spacing, .radius = radiusSmall, .unit = NULL };
	tiles[27].color = GREEN;

	tiles[28] = (Tile){ .nextTile = &tiles[29], .posX = boardLeft + 3 * radiusSmall + 3 * spacing, .posY = boardTop + 1 * radiusSmall + 1 * spacing, .radius = radiusSmall };
	tiles[28].color = BLUE;

	tiles[29].nextTile = NULL;
	tiles[29].posX = boardLeft + 3 * radiusSmall + 3 * spacing;
	tiles[29].posY = boardTop + 2 * radiusSmall + 2 * spacing;
	tiles[29].radius = radiusSmall;
	tiles[29].color = BLUE;

	tiles[30].nextTile = &tiles[31];
	tiles[30].posX = boardLeft + 5 * radiusSmall + 5 * spacing;
	tiles[30].posY = boardTop + 3 * radiusSmall + 3 * spacing;
	tiles[30].radius = radiusSmall;
	tiles[30].color = YELLOW;

	tiles[31].nextTile = NULL;
	tiles[31].posX = boardLeft + 4 * radiusSmall + 4 * spacing;
	tiles[31].posY = boardTop + 3 * radiusSmall + 3 * spacing;
	tiles[31].radius = radiusSmall;
	tiles[31].color = YELLOW;

	/* Teams */
	teams[0] = (Team){ .finish = &tiles[24], .name = "Red" };
	teams[0].spawn[0] = &tiles[37];
	teams[0].spawn[1] = &tiles[38];
	teams[0].spawn[2] = &tiles[32];
	teams[0].spawn[3] = &tiles[36];
	teams[0].playerClip = (SDL_Rect){ .x = 0, .y = 0, .w = playerWidth, .h = playerHeight };

	teams[1] = (Team){ .finish = &tiles[26], .name = "Green" };
	teams[1].spawn[0] = &tiles[33];
	teams[1].spawn[1] = &tiles[39];
	teams[1].spawn[2] = &tiles[40];
	teams[1].spawn[3] = &tiles[41];
	teams[1].playerClip = (SDL_Rect) { .x = playerWidth, .y = 0, .w = playerWidth, .h = playerHeight };

	teams[2] = (Team){ .finish = &tiles[28], .name = "Blue" };
	teams[2].spawn[0] = &tiles[43];
	teams[2].spawn[1] = &tiles[34];
	teams[2].spawn[2] = &tiles[44];
	teams[2].spawn[3] = &tiles[42];
	teams[2].playerClip = (SDL_Rect){ .x = 0, .y = playerHeight, .w = playerWidth, .h = playerHeight };
	
	teams[3] = (Team){ .finish = &tiles[30], .name = "Yellow" };
	teams[3].spawn[0] = &tiles[47];
	teams[3].spawn[1] = &tiles[46];
	teams[3].spawn[2] = &tiles[45];
	teams[3].spawn[3] = &tiles[35];
	teams[3].playerClip = (SDL_Rect){ .x = playerWidth, .y = playerHeight, .w = playerWidth, .h = playerHeight };

	for (int i = 0; i < nrOfTeams; i++)
	{
		for (int j = 0; j < teamSize; j++)
		{
			teams[i].units[j] = (Unit){ .team = &teams[i], .position = teams[i].spawn[j] };
			teams[i].units[j].position->unit = &teams[i].units[j];
		}
	}

	gameMsgTexture = (Texture*)malloc(sizeof(Texture));
	*gameMsgTexture = (Texture){ .height = 0, .width = 0, .texture = NULL };
	
	turn = 0;
	setGamePhase(ROLL);

	renderHandler = &renderGame;
	eventHandler = &handleGameEvent;
	unloadHandler = &unloadGame;

	return success;
}

void unloadGame()
{
	freeTexture(playersSprite);
	free(playersSprite);
	freeTexture(gameMsgTexture);
	free(gameMsgTexture);
	freeTexture(&(die->texture));
	free(die);
}

Uint8 selectedColor = 0xFF;
void renderGame()
{
	/* Render game message */
	renderTexture(gameMsgTexture, renderer, 10, 10, NULL, 0, NULL, SDL_FLIP_NONE);

	/* Render tiles */
	for (int i = 0; i < tilesSize; i++)
	{
		SDL_Rect tile = { tiles[i].posX, tiles[i].posY, tiles[i].radius, tiles[i].radius };
		SDL_SetRenderDrawColor(renderer, tiles[i].color.r, tiles[i].color.g, tiles[i].color.b, tiles[i].color.a);
		SDL_RenderFillRect(renderer, &tile);
	}

	/* Render sprites */
	for (int i = 0; i < nrOfTeams; i++)
	{
		for (int j = 0; j < teamSize; j++)
		{
			int posX = (teams[i].units[j].position->posX + (teams[i].units[j].position->radius / 2)) - (playerWidth / 2);
			int posY = (teams[i].units[j].position->posY + (teams[i].units[j].position->radius / 2)) - (playerHeight / 2);
			renderTexture(playersSprite, renderer, posX, posY, &teams[i].playerClip, 0, NULL, SDL_FLIP_NONE);
		}


		SDL_SetRenderDrawColor(renderer, selectedColor, selectedColor, selectedColor, 0xFF);
		SDL_Rect rect = 
		{ 
			teams[turn].units[selectedUnitIndex].position->posX + (((teams[turn].units[selectedUnitIndex].position->radius) / 2) - 10 / 2), 
			teams[turn].units[selectedUnitIndex].position->posY, 10, 10 
		};
		SDL_RenderFillRect(renderer, &rect);
		if (selectedColor == 0) {
			selectedColor = 0xFF;
		} else {
			selectedColor--;
		}
	}

	/* Animations */
	if (dieAnimation.remainingFrames >= 0)
	{
		int w = die->clip.w;
		int x = (dieAnimation.remainingFrames * w) % (6 * w);
		SDL_Rect clip = (SDL_Rect){ .x = x, .y = 0, .w = w, .h = die->clip.h };
		renderTexture(&die->texture, renderer, 50, 50, &clip, 0, NULL, SDL_FLIP_NONE);
		if (dieAnimation.remainingFrames == 0) {
			pauseInput = 0;
		}
		
		dieAnimation.remainingFrames--;
	}
	else if (phase == MOVE) {
		renderTexture(&die->texture, renderer, 50, 50, &die->clip, 0, NULL, SDL_FLIP_NONE);
	}
}

int handleGameEvent(SDL_Event* e)
{
	int success = 1;
	if (e->type == SDL_KEYDOWN)
	{
		if (e->key.keysym.sym == SDLK_ESCAPE)
		{
			unloadGame();
			if (!loadMainMenu())
			{
				printf("Failed to load main menu!\n");
				success = 0;
			}
		}
		else if (!pauseInput)
		{
			if (phase == ROLL)
			{
				if (e->key.keysym.sym == SDLK_SPACE)
				{
					castDie(die);
					//dieAnimation.die = die;

					// TODO: why is this not 1 sec???
					dieAnimation.remainingFrames = 60;
					pauseInput = 1;
					setGamePhase(MOVE);
				}
			}
			else if (phase == MOVE)
			{
				if (e->key.keysym.sym == SDLK_RETURN)
				{
 					// move player
					if ((teams[turn].units[selectedUnitIndex].position->type == SPAWN && (die->currentValue == 1 || die->currentValue == 6)) || teams[turn].units[selectedUnitIndex].position->type != SPAWN)
					{
						int moveLegal = 0;
						Tile* origPos = teams[turn].units[selectedUnitIndex].position;

						// TODO: have to finish on the finish tile
						for (int i = 0; i < die->currentValue; i++) {
							teams[turn].units[selectedUnitIndex].position = teams[turn].units[selectedUnitIndex].position->nextTile;
						}

						// if tile was empty
						if (teams[turn].units[selectedUnitIndex].position->unit == NULL) 
						{
							moveLegal = 1;
						}
						else 
						{
							// if tile is occupied by team member
							if (strcmp(teams[turn].units[selectedUnitIndex].position->unit->team->name, teams[turn].name) == 0)
							{
								
							}
							else 
							{
								for (int i = 0; i < 4; i++)
								{
									if (teams[turn].units[selectedUnitIndex].position->unit->team->spawn[i]->unit == NULL)
									{
										teams[turn].units[selectedUnitIndex].position->unit->position = teams[turn].units[selectedUnitIndex].position->unit->team->spawn[i];
										i = 4;
									}
								}
								

								moveLegal = 1;
							}
						}

						if (moveLegal)
						{
							teams[turn].units[selectedUnitIndex].position->unit = &teams[turn].units[selectedUnitIndex];
							origPos->unit = NULL;
							turn = (turn + 1) % nrOfTeams;
							setGamePhase(ROLL);
						}
						else
						{
							teams[turn].units[selectedUnitIndex].position = origPos;
						}
					}
				}
				else if (e->key.keysym.sym == SDLK_s)
				{
					turn = (turn + 1) % nrOfTeams;
					setGamePhase(ROLL);
				}
				else if (e->key.keysym.sym == SDLK_RIGHT) 
				{
					selectedUnitIndex = (selectedUnitIndex + 1) % teamSize;
				}
				else if (e->key.keysym.sym == SDLK_LEFT) 
				{
					selectedUnitIndex--;
					if (selectedUnitIndex < 0) {
						selectedUnitIndex = teamSize - 1;
					}
				}
			}
		}
	}

	return success;
}

void setGamePhase(GamePhase p)
{
	switch (p)
	{
		case ROLL:
			phase = ROLL;
			selectedUnitIndex = 0;
			char msg1[6 + 38 + 30] = "Team ";
			strcat_s(msg1, 74, teams[turn].name);
			char msg2[38] = "'s turn. Press Space to roll the die.";
			strcat_s(msg1, 59, msg2);
			loadFromRenderedText(gameMsgTexture, renderer, font, msg1, (SDL_Color){ 0, 0, 0 });
			break;
		case MOVE:
			phase = MOVE;
			char msg3[6 + 23 + 30] = "Team ";
			strcat_s(msg3, 59, teams[turn].name);
			char msg4[23] = "'s turn. Move a piece.";
			strcat_s(msg3, 59, msg4);
			loadFromRenderedText(gameMsgTexture, renderer, font, msg3, (SDL_Color){ 0, 0, 0 });
			break;
	}
}