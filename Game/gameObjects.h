#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

#include <stdlib.h>
#include <SDL_pixels.h>
#include <texture.h>

#define teamSize 4
#define nrOfTeams 4

typedef struct Team Team;
typedef struct Unit Unit;
typedef struct Tile Tile;
typedef struct Die Die;
typedef struct DieAnimation DieAnimation;

int castDie(Die* die);

typedef enum { SPAWN, FINISH, RING } TileType;

struct Die
{
	int sides;
	int currentValue;
	Texture texture;
	SDL_Rect clip;
};

struct DieAnimation
{
	Die* die;
	int remainingFrames;
};

struct Unit
{
	Team* team;
	Tile* position;
};

struct Team
{
	//Tile* start;
	Tile* spawn[teamSize];
	Tile* finish;
	Unit units[teamSize];
	SDL_Rect playerClip;
	char* name;
};

struct Tile
{
	int posX;
	int posY;
	int radius;
	Tile* nextTile;

	// TODO: make this pointer
	SDL_Color color;
	Unit* unit;

	TileType type;
};

int castDie(Die* die)
{
	// The random must be seeded first using srand
	die->currentValue = (rand() % die->sides) + 1;
	die->clip.x = die->clip.w * (die->currentValue - 1);
}

#endif