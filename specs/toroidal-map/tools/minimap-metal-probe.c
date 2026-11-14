/*
 * Exercise integer upscaling of a logical minimap raster through SDL3.
 * The printed renderer name identifies whether a hardware backend was used.
 * License: GNU General Public License v2.0 or later.
 */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdint.h>

int main(void)
{
	if(!SDL_Init(SDL_INIT_VIDEO)) { fprintf(stderr, "init: %s\n", SDL_GetError()); return 2; }
	SDL_Window *window = SDL_CreateWindow("VCMI minimap scaling probe", 150, 16, SDL_WINDOW_HIDDEN);
	SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, NULL) : NULL;
	if(!renderer) { fprintf(stderr, "renderer: %s\n", SDL_GetError()); return 2; }
	printf("renderer=%s\n", SDL_GetRendererName(renderer));
	const int sizes[][2] = {{36,150},{60,100},{72,72},{144,73},{144,150},{252,144},{252,150}};
	int failures = 0;
	for(unsigned i=0; i<sizeof(sizes)/sizeof(sizes[0]); ++i)
	{
		const int map = sizes[i][0], display = sizes[i][1];
		SDL_Surface *source = SDL_CreateSurface(display, 1, SDL_PIXELFORMAT_RGBA32);
		SDL_Surface *image = NULL;
		SDL_Texture *src = NULL, *target = NULL;
		if(!source) return 2;
		for(int x=0; x<display; ++x)
		{
			int tile=(int)(((2LL*x+1)*map-1)/(2LL*display));
			if(!SDL_WriteSurfacePixel(source, x, 0, tile, 0, 0, 255)) return 2;
		}
		src = SDL_CreateTextureFromSurface(renderer, source);
		target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, display*2, 1);
		if(!src || !target || !SDL_SetTextureScaleMode(src, SDL_SCALEMODE_NEAREST) || !SDL_SetRenderTarget(renderer, target) || !SDL_RenderTexture(renderer, src, NULL, NULL)) { fprintf(stderr,"draw: %s\n",SDL_GetError()); return 2; }
		image = SDL_RenderReadPixels(renderer, NULL);
		if(!image) return 2;
		int count = 0;
		for(int x=0; x<display*2; ++x)
		{
			Uint8 r,g,b,a;
			if(!SDL_ReadSurfacePixel(image,x,0,&r,&g,&b,&a)) return 2;
			const int expected=(int)(((2LL*(x/2)+1)*map-1)/(2LL*display));
			if(r!=expected) { if(count<5) printf("%d/%d pixel=%d actual=%u expected=%d\n",map,display,x,r,expected); ++count; }
		}
		printf("%d/%d mismatches=%d\n",map,display,count); failures += count;
		SDL_SetRenderTarget(renderer,NULL); SDL_DestroySurface(image); SDL_DestroyTexture(target); SDL_DestroyTexture(src); SDL_DestroySurface(source);
	}
	SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
	return failures ? 1 : 0;
}
