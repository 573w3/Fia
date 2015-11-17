#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

typedef struct Texture
{
	SDL_Texture* texture;
	int width;
	int height;
} Texture;

// Texture functions
void freeTexture(Texture* texture);
int loadTextureFromFile(Texture* texture, SDL_Renderer* renderer, char* path);
void renderTexture(Texture* texture, SDL_Renderer* renderer, int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip);
void setTextureAlpha(Texture* texture, Uint8 alpha);
void setTextureBlendMode(Texture* texture, SDL_BlendMode blending);
void setTextureColor(Texture* texture, Uint8 red, Uint8 green, Uint8 blue);

void freeTexture(Texture* texture)
{
	//Free texture if it exists
	if (texture->texture != NULL)
	{
		SDL_DestroyTexture(texture->texture);
		texture->texture = NULL;
		texture->width = 0;
		texture->height = 0;
	}
}

int loadTextureFromFile(Texture* texture, SDL_Renderer* renderer, char* path)
{
	//Get rid of preexisting texture
	freeTexture(texture);

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path);
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
		}
		else
		{
			//Get image dimensions
			texture->width = loadedSurface->w;
			texture->height = loadedSurface->h;
			texture->texture = newTexture;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	return texture->texture != NULL;
}

int loadFromRenderedText(Texture* texture, SDL_Renderer* renderer, TTF_Font* font, char* textureText, SDL_Color textColor)
{
	//Get rid of preexisting texture
	freeTexture(texture);

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(font, textureText, textColor);
	if (textSurface == NULL)
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		//Create texture from surface pixels
		texture->texture = SDL_CreateTextureFromSurface(renderer, textSurface);
		if (texture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			texture->width = textSurface->w;
			texture->height = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}

	//Return success
	return texture->texture != NULL;
}

void setTextureColor(Texture* texture, Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(texture->texture, red, green, blue);
}

void setTextureBlendMode(Texture* texture, SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(texture->texture, blending);
}

void setTextureAlpha(Texture* texture, Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(texture->texture, alpha);
}

void renderTexture(Texture* texture, SDL_Renderer* renderer, int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, texture->width, texture->height };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(renderer, texture->texture, clip, &renderQuad, angle, center, flip);
}
#endif