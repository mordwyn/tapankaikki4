#include "CSplash.h"
#include "common/error.h"
#include "common/files.h"

void CSplash::ShowSplash(const char* aImagePath,const char* aIcon,const char* aCaption)
{
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
	{
		error("Unable to initialize SDL: %s \n", SDL_GetError());
	}

	SDL_Surface* bitmap = SDL_LoadBMP(getdatapath(std::string(aImagePath)).c_str());
	if (bitmap == NULL)
	{
		error("Unable to load splash.\n");
	}

	SDL_Window* window = SDL_CreateWindow(aCaption ? aCaption : "",
	                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      bitmap->w, bitmap->h,
	                                      SDL_WINDOW_BORDERLESS);
	if (window == NULL)
	{
		error("Unable to create splash window: %s\n", SDL_GetError());
	}

	if (aIcon != NULL && exists(aIcon))
	{
		SDL_Surface* icon = SDL_LoadBMP(aIcon);
		if (icon)
		{
			SDL_SetWindowIcon(window, icon);
			SDL_FreeSurface(icon);
		}
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, bitmap);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	// Keep the splash visible briefly; SDL2 uses a separate window per surface,
	// so it is torn down before the game opens its own window.
	SDL_PumpEvents();
	SDL_Delay(1000);

	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(bitmap);
}
