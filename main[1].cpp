
#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define FPS 150
#define TARGET_TIME (1000/FPS)

#define SCREEN_WIDTH	680
#define SCREEN_HEIGHT	520
#define PLATFORM_DIFF	70	
#define POCZATKOWA_X_GR (SCREEN_WIDTH - (SCREEN_WIDTH * 0.96))	// 0.95 / 0.15
#define POCZATKOWA_Y_GR (SCREEN_HEIGHT - 43)	// poczatkowa 43 / 403
#define LICZBA_ETAPOW 3
#define LICZBA_BECZEK 6

#define PREDKOSC_POZIOMA		160.00
#define PREDKOSC_PIONOWA		80.00
#define PREDKOSC_GRAWITACYJNA	120.00
#define PREDKOSC_BECZKI			100.0

#define PLATFORMS_ONE	7	// iloscc platform na etap
#define PLATFORMS_TWO	10
#define PLATFORMS_THREE	10	
#define LADDERS_ONE		6	// ilosc drabin na etap
#define LADDERS_TWO		6	
#define LADDERS_THREE	7	

typedef struct {
	double x;
	double y;
	SDL_Rect rect;
}Person;

typedef struct {
	SDL_Rect rect;
} Platform;

typedef struct {
	SDL_Rect rect;
} Ladder;

typedef struct {
	float x;
	float y;
	int pkt_kontrolny;
	SDL_Rect rect;
} Barrel;

typedef struct {
	int x, y, w, h;
	bool mysz;
} Obszar;

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};


// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) 
// bπdü poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokπta o d≥ugoúci bokÛw l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

int checkCollision(SDL_Rect rect1, SDL_Rect rect2) {
	return rect1.x < rect2.x + rect2.w &&
		rect1.x + rect1.w > rect2.x &&
		rect1.y < rect2.y + rect2.h &&
		rect1.y + rect1.h > rect2.y;
};

bool checkCollisionAtPosition(const Platform& platform, const Ladder& drabina, int x, int y) {
	SDL_Rect positionRect = { x, y, 1, 1 };  // Tworzymy jednopikselowy prostokπt w danym miejscu

	// Sprawdzenie kolizji z platformπ i drabinπ
	return checkCollision(positionRect, platform.rect) && checkCollision(positionRect, drabina.rect);
}

bool checkCollisionAtPositionSide(const Platform& platform, Person gracz, int x, int y) {
	SDL_Rect positionRect = { x, y, 1, 1 };  // Tworzymy jednopikselowy prostokπt w danym miejscu

	// Sprawdzenie kolizji z platformπ  i graczem od boku
	return checkCollision(positionRect, platform.rect) && checkCollision(positionRect, gracz.rect);
}

int czyMyszNadObszarem(int x, int y, const Obszar* obszar) {
	return (x >= obszar->x && x <= obszar->x + obszar->w &&
		y >= obszar->y && y <= obszar->y + obszar->h);
}

bool restartEtapu(double* worldTime, Person* gracz) {
	*worldTime = 0;

	gracz->x = (double)(POCZATKOWA_X_GR);
	gracz->y = (double)(POCZATKOWA_Y_GR);

	gracz->rect.x = (int)gracz->x;
	gracz->rect.y = (int)gracz->y;

	return true;
};
// PIERWSZY ETAP
void rysujPierwszy(SDL_Surface* screen, Platform platforms_one[PLATFORMS_ONE], SDL_Surface* drabina, SDL_Surface* flaga, SDL_Surface* ksiezniczka, SDL_Surface* beczka, Uint32 zielony, Uint32 czerwony) {

	for (int i = 0; i < PLATFORMS_ONE; i++) {
		DrawRectangle(screen, platforms_one[i].rect.x, platforms_one[i].rect.y, platforms_one[i].rect.w, platforms_one[i].rect.h, zielony, czerwony);
	}

	// DRABINA
	DrawSurface(screen, drabina, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 50);
	DrawSurface(screen, drabina, 100, SCREEN_HEIGHT - 120);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 190);
	DrawSurface(screen, drabina, 100, SCREEN_HEIGHT - 260);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 330);

	// KONIEC ETAPU
	DrawRectangle(screen, 200, SCREEN_HEIGHT - 420, 100, 10, zielony, czerwony);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 400, SCREEN_HEIGHT - 400);	
	DrawSurface(screen, flaga, 25, SCREEN_HEIGHT - 30);
	DrawSurface(screen, ksiezniczka, SCREEN_WIDTH - 450, SCREEN_HEIGHT - 440);

	// BECZKI
	DrawRectangle(screen, 60, SCREEN_HEIGHT - 400, 40, 40, czerwony, czerwony);	// wyrzuca beczki (xy lewy gÛra)

}
void Beczki_Jeden(Barrel beczki[], bool dead, int* ilosc_beczek, double distanceBeczki, SDL_Surface* screen, SDL_Surface* beczka, Uint32 zielony, int* opoznienie_beczek) {
	if (!dead) {
		for (int i = 0; i < *ilosc_beczek; i++) {

			if (beczki[i].pkt_kontrolny == 1) {			
				beczki[i].x += distanceBeczki;
				if (beczki[i].x >= (SCREEN_WIDTH - PLATFORM_DIFF)) {
					beczki[i].x = SCREEN_WIDTH - PLATFORM_DIFF;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 2) {
				beczki[i].y += distanceBeczki;
				if (beczki[i].y >= (SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 35 + 5)) {
					beczki[i].y = SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 35 + 5;
					beczki[i].x -= distanceBeczki;	
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 3) {	
				beczki[i].x -= distanceBeczki;
				if (beczki[i].x <= (PLATFORM_DIFF - 25)) {
					beczki[i].x = PLATFORM_DIFF - 25;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 4) {
				beczki[i].y += distanceBeczki;
				if (beczki[i].y >= (SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 35 + 5)) {
					beczki[i].y = SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 35 + 5;
					beczki[i].x += distanceBeczki;	
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 5) {
				beczki[i].x += distanceBeczki;
				if (beczki[i].x >= (SCREEN_WIDTH - PLATFORM_DIFF)) {
					beczki[i].x = SCREEN_WIDTH - PLATFORM_DIFF;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 6) {
				beczki[i].y += distanceBeczki;
				if (beczki[i].y >= (SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 35 + 5)) {
					beczki[i].y = SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 35 + 5;
					beczki[i].x -= distanceBeczki;	
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 7) {	
				beczki[i].x -= distanceBeczki;
				if (beczki[i].x <= (PLATFORM_DIFF - 25)) {
					beczki[i].x = PLATFORM_DIFF - 25;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 8) {
				beczki[i].y += distanceBeczki;
				if (beczki[i].y >= (SCREEN_HEIGHT - PLATFORM_DIFF - 35 + 5)) {
					beczki[i].y = SCREEN_HEIGHT - PLATFORM_DIFF - 35 + 5;
					beczki[i].x += distanceBeczki;	
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 9) {
				beczki[i].x += distanceBeczki;
				if (beczki[i].x >= (SCREEN_WIDTH - PLATFORM_DIFF)) {
					beczki[i].x = SCREEN_WIDTH - PLATFORM_DIFF;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 10) {
				beczki[i].y += distanceBeczki;
				if (beczki[i].y >= (SCREEN_HEIGHT - 35 + 5)) {
					beczki[i].y = SCREEN_HEIGHT - 35 + 5;
					beczki[i].x -= distanceBeczki;	
					beczki[i].pkt_kontrolny += 1;
				}
			}
			else if (beczki[i].pkt_kontrolny == 11) {	
				beczki[i].x -= distanceBeczki;
				if (beczki[i].x <= (70)) {
					beczki[i].x = 70;
					beczki[i].y += distanceBeczki;
					beczki[i].pkt_kontrolny = 1;
					beczki[i].y = (SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 35 + 5);
				}
			}

			DrawSurface(screen, beczka, beczki[i].x + 12, beczki[i].y + 12 - 5);

			beczki[i].rect.x = beczki[i].x;
			beczki[i].rect.y = beczki[i].y;
		}
	}
	else {
		*ilosc_beczek = 0;
		*opoznienie_beczek = 3;
		for (int i = 0; i < LICZBA_BECZEK; i++) {
			beczki[i].pkt_kontrolny = 1;
			beczki[i].x = 100;
			beczki[i].y = SCREEN_HEIGHT - 380;

			beczki[i].rect.x = beczki[i].x;
			beczki[i].rect.y = beczki[i].y;
		}
	}
}
void platformCollision_Jeden(Person gracz, Platform platforms_one[PLATFORMS_ONE], bool* onPlatform, bool isJumping, bool* isFalling, double* distanceY, double* distanceX, bool onLadder) {


	for (int i = 0; i < PLATFORMS_ONE; i++) {
		if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, platforms_one[i].rect)) {
			*onPlatform = true;
			if (isJumping) {
				*distanceY = 0;
			}
			else {
				*isFalling = false;
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_one[i], gracz, gracz.rect.x, gracz.rect.y + j)) { // kolizja boczna z lewej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_one[i], gracz, gracz.rect.x + 24, gracz.rect.y + j)) { // kolizja boczna z prawej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			break;
		}
	}
}
void collisionBoth_Jeden(Person gracz, Platform platforms_one[PLATFORMS_ONE], Ladder laddres_one[LADDERS_ONE], bool keyUp, bool* onBoth) {

	if (keyUp) {    // W G”RE
		for (int i = 0; i < PLATFORMS_ONE; i++) {
			for (int j = 0; j < LADDERS_ONE; j++) {
				if (checkCollisionAtPosition(platforms_one[i], laddres_one[j], gracz.rect.x, gracz.rect.y)) {
					*onBoth = true;
				}
			}
		}
	}
	else {   // W D”£
		for (int i = 0; i < PLATFORMS_ONE; i++) {
			for (int j = 0; j < LADDERS_ONE; j++) {
				if (checkCollisionAtPosition(platforms_one[i], laddres_one[j], gracz.rect.x + 12, gracz.rect.y + 24)) {
					*onBoth = true;
				}
			}
		}
	}
}
// DRUGI ETAP
void rysujDrugi(SDL_Surface* screen, Platform platforms_two[PLATFORMS_TWO], SDL_Surface* drabina, SDL_Surface* flaga, SDL_Surface* ksiezniczka, Uint32 zielony, Uint32 czerwony) {

	for (int i = 0; i < PLATFORMS_TWO; i++) {
		DrawRectangle(screen, platforms_two[i].rect.x, platforms_two[i].rect.y, platforms_two[i].rect.w, platforms_two[i].rect.h, zielony, czerwony);
	}

	// DRABINA
	DrawSurface(screen, drabina, PLATFORM_DIFF + 50, SCREEN_HEIGHT - 50);
	DrawSurface(screen, drabina, SCREEN_WIDTH / 2 - 10, SCREEN_HEIGHT - 120);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 210, SCREEN_HEIGHT - 190);
	DrawSurface(screen, drabina, 180, SCREEN_HEIGHT - 260);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 210, SCREEN_HEIGHT - 330);

	// KONIEC ETAPU
	DrawRectangle(screen, 200, SCREEN_HEIGHT - 420, 100, 10, zielony, czerwony);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 400, SCREEN_HEIGHT - 400);	// ostatnia drabina
	DrawSurface(screen, flaga, 20, SCREEN_HEIGHT - 30);
	DrawSurface(screen, ksiezniczka, SCREEN_WIDTH - 450, SCREEN_HEIGHT - 440);
}
void Beczki_Dwa(Barrel beczki_Dwa[], bool dead, int* ilosc_beczek, double distanceBeczki, SDL_Surface* screen, SDL_Surface* beczka, Uint32 zielony, int* opoznienie_beczek) {
	if (!dead) {
		
		if (beczki_Dwa[0].pkt_kontrolny == 1) {		// 1
			beczki_Dwa[0].y += distanceBeczki;
			if (beczki_Dwa[0].y >= (SCREEN_HEIGHT - PLATFORM_DIFF - 25)) {
				beczki_Dwa[0].y = (SCREEN_HEIGHT - PLATFORM_DIFF - 25);
				beczki_Dwa[0].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Dwa[0].pkt_kontrolny == 2) {
			beczki_Dwa[0].y -= distanceBeczki;
			if (beczki_Dwa[0].y <= (SCREEN_HEIGHT - 400)) {
				beczki_Dwa[0].y = (SCREEN_HEIGHT - 400);
				beczki_Dwa[0].pkt_kontrolny = 1;
			}
		}

		if (beczki_Dwa[1].pkt_kontrolny == 1) {		// 2
			beczki_Dwa[1].x += distanceBeczki;
			if (beczki_Dwa[1].x >= (SCREEN_WIDTH - 35)) {
				beczki_Dwa[1].x = (SCREEN_WIDTH - 35);
				beczki_Dwa[1].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Dwa[1].pkt_kontrolny == 2) {
			beczki_Dwa[1].x -= distanceBeczki;
			if (beczki_Dwa[1].x <= 10) {
				beczki_Dwa[1].x = 10;
				beczki_Dwa[1].pkt_kontrolny = 1;
			}
		}

		if (beczki_Dwa[2].pkt_kontrolny == 1) {		// 3
			beczki_Dwa[2].y += distanceBeczki;
			if (beczki_Dwa[2].y >= (SCREEN_HEIGHT - 35)) {
				beczki_Dwa[2].y = (SCREEN_HEIGHT - 35);
				beczki_Dwa[2].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Dwa[2].pkt_kontrolny == 2) {
			beczki_Dwa[2].y -= distanceBeczki;
			if (beczki_Dwa[2].y <= (SCREEN_HEIGHT - 400)) {
				beczki_Dwa[2].y = (SCREEN_HEIGHT - 400);
				beczki_Dwa[2].pkt_kontrolny = 1;
			}
		}
		if (beczki_Dwa[3].pkt_kontrolny == 1) {		// 4
			beczki_Dwa[3].x += distanceBeczki;
			if (beczki_Dwa[3].x >= (SCREEN_WIDTH - 35)) {
				beczki_Dwa[3].x = (SCREEN_WIDTH - 35);
				beczki_Dwa[3].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Dwa[3].pkt_kontrolny == 2) {
			beczki_Dwa[3].x -= distanceBeczki;
			if (beczki_Dwa[3].x <= ( PLATFORM_DIFF - 10)) {
				beczki_Dwa[3].x = PLATFORM_DIFF - 10;
				beczki_Dwa[3].pkt_kontrolny = 1;
			}
		}
		if (beczki_Dwa[4].pkt_kontrolny == 1) {		// 2
			beczki_Dwa[4].x += distanceBeczki;
			if (beczki_Dwa[4].x >= (SCREEN_WIDTH - 35)) {
				beczki_Dwa[4].x = (SCREEN_WIDTH - 35);
				beczki_Dwa[4].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Dwa[4].pkt_kontrolny == 2) {
			beczki_Dwa[4].x -= distanceBeczki;
			if (beczki_Dwa[4].x <= 10) {
				beczki_Dwa[4].x = 10;
				beczki_Dwa[4].pkt_kontrolny = 1;
			}
		}
		for (int i = 0; i < LICZBA_BECZEK; i++) {
			DrawSurface(screen, beczka, beczki_Dwa[i].x + 12, beczki_Dwa[i].y + 12);

			beczki_Dwa[i].rect.x = beczki_Dwa[i].x;
			beczki_Dwa[i].rect.y = beczki_Dwa[i].y;
		}
	}
}
void platformCollision_Dwa(Person gracz, Platform platforms_two[PLATFORMS_TWO], bool* onPlatform, bool isJumping, bool* isFalling, double* distanceY, double* distanceX) {
	for (int i = 0; i < PLATFORMS_TWO; i++) {
		if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, platforms_two[i].rect)) {
			*onPlatform = true;
			if (isJumping) {
				*distanceY = 0;
			}
			else {
				*isFalling = false;
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_two[i], gracz, gracz.rect.x, gracz.rect.y + j)) { //bÍdzie siÍ obniøa≥o, kolizja boczna z lewej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_two[i], gracz, gracz.rect.x + 24, gracz.rect.y + j)) { //bÍdzie siÍ obniøa≥o, kolizja boczna z prawej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			break;
		}
	}
}
void collisionBoth_Dwa(Person gracz, Platform platforms_two[PLATFORMS_TWO], Ladder laddres_two[LADDERS_TWO], bool keyUp, bool* onBoth) {

	if (keyUp) {    // W G”RE
		for (int i = 0; i < PLATFORMS_TWO; i++) {
			for (int j = 0; j < LADDERS_TWO; j++) {
				if (checkCollisionAtPosition(platforms_two[i], laddres_two[j], gracz.rect.x, gracz.rect.y)) {
					*onBoth = true;
				}
			}
		}
	}
	else {   // W D”£
		for (int i = 0; i < PLATFORMS_TWO; i++) {
			for (int j = 0; j < LADDERS_TWO; j++) {
				if (checkCollisionAtPosition(platforms_two[i], laddres_two[j], gracz.rect.x + 12, gracz.rect.y + 24)) {
					*onBoth = true;
				}
			}
		}
	}
}
// ETAP TRZECI 
void rysujTrzeci(SDL_Surface* screen, Platform platforms_three[PLATFORMS_THREE], SDL_Surface* drabina, SDL_Surface* flaga, SDL_Surface* ksiezniczka, Uint32 zielony, Uint32 czerwony) {

	for (int i = 0; i < PLATFORMS_THREE; i++) {
		DrawRectangle(screen, platforms_three[i].rect.x, platforms_three[i].rect.y, platforms_three[i].rect.w, platforms_three[i].rect.h, zielony, czerwony);
	}

	// DRABINA
	DrawSurface(screen, drabina, SCREEN_WIDTH - 70, SCREEN_HEIGHT - 50);
	DrawSurface(screen, drabina, SCREEN_WIDTH / 2 + 20, SCREEN_HEIGHT - 120);
	DrawSurface(screen, drabina, 100, SCREEN_HEIGHT - 190);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 260);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 240, SCREEN_HEIGHT - 330);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 330, SCREEN_HEIGHT - 330);

	// KONIEC ETAPU
	DrawRectangle(screen, 200, SCREEN_HEIGHT - 420, 100, 10, zielony, czerwony);
	DrawSurface(screen, drabina, SCREEN_WIDTH - 400, SCREEN_HEIGHT - 400);	
	DrawSurface(screen, flaga, 20, SCREEN_HEIGHT - 30);
	DrawSurface(screen, ksiezniczka, SCREEN_WIDTH - 450, SCREEN_HEIGHT - 440);
}

void Beczki_Trzy(Barrel beczki_Trzy[], bool dead, int* ilosc_beczek, double distanceBeczki, SDL_Surface* screen, SDL_Surface* beczka, Uint32 zielony, int* opoznienie_beczek) {
	if (!dead) {
		if (beczki_Trzy[0].pkt_kontrolny == 1) {
			beczki_Trzy[0].x -= distanceBeczki;
			if (beczki_Trzy[0].x <= (SCREEN_WIDTH / 2)) {
				beczki_Trzy[0].x = (SCREEN_WIDTH / 2);
				beczki_Trzy[0].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Trzy[0].pkt_kontrolny == 2) {
			beczki_Trzy[0].x += distanceBeczki;
			if (beczki_Trzy[0].x >= (SCREEN_WIDTH -50)) {
				beczki_Trzy[0].x = (SCREEN_WIDTH - 50);
				beczki_Trzy[0].pkt_kontrolny = 1;
			}
		}
		if (beczki_Trzy[1].pkt_kontrolny == 1) {	// 2
			beczki_Trzy[1].x -= distanceBeczki;
			if (beczki_Trzy[1].x <= 70) {
				beczki_Trzy[1].x = 70;
				beczki_Trzy[1].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Trzy[1].pkt_kontrolny == 2) {
			beczki_Trzy[1].x += distanceBeczki;
			if (beczki_Trzy[1].x >= (SCREEN_WIDTH / 2 + 30)) {
				beczki_Trzy[1].x = (SCREEN_WIDTH / 2 + 30);
				beczki_Trzy[1].pkt_kontrolny = 1;
			}
		}
		if (beczki_Trzy[2].pkt_kontrolny == 1) {	// 3
			beczki_Trzy[2].x += distanceBeczki;
			if (beczki_Trzy[2].x >= ( SCREEN_WIDTH - 90)) {
				beczki_Trzy[2].x = ( SCREEN_WIDTH - 90);
				beczki_Trzy[2].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Trzy[2].pkt_kontrolny == 2) {
			beczki_Trzy[2].x -= distanceBeczki;
			if (beczki_Trzy[2].x <= 50) {
				beczki_Trzy[2].x = 50;
				beczki_Trzy[2].pkt_kontrolny = 1;
			}
		}
		if (beczki_Trzy[3].pkt_kontrolny == 1) {	// 4
			beczki_Trzy[3].x += distanceBeczki;
			if (beczki_Trzy[3].x >= (SCREEN_WIDTH - 55)) {
				beczki_Trzy[3].x = (SCREEN_WIDTH - 55);
				beczki_Trzy[3].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Trzy[3].pkt_kontrolny == 2) {
			beczki_Trzy[3].x -= distanceBeczki;
			if (beczki_Trzy[3].x <= (SCREEN_WIDTH / 2 - 100)) {
				beczki_Trzy[3].x = (SCREEN_WIDTH /2 - 100);
				beczki_Trzy[3].pkt_kontrolny = 1;
			}
		}
		if (beczki_Trzy[4].pkt_kontrolny == 1) {	// 5
			beczki_Trzy[4].x += distanceBeczki;
			if (beczki_Trzy[4].x >= (SCREEN_WIDTH / 2 + 100)) {
				beczki_Trzy[4].x = (SCREEN_WIDTH/2 +100);
				beczki_Trzy[4].pkt_kontrolny = 2;
			}
		}
		else if (beczki_Trzy[4].pkt_kontrolny == 2) {
			beczki_Trzy[4].x -= distanceBeczki;
			if (beczki_Trzy[4].x <= (SCREEN_WIDTH / 2 - 100)) {
				beczki_Trzy[4].x = (SCREEN_WIDTH/2 -100);
				beczki_Trzy[4].pkt_kontrolny = 1;
			}
		}
		for (int i = 0; i < LICZBA_BECZEK; i++) {
			DrawSurface(screen, beczka, beczki_Trzy[i].x + 12, beczki_Trzy[i].y + 12);

			beczki_Trzy[i].rect.x = beczki_Trzy[i].x;
			beczki_Trzy[i].rect.y = beczki_Trzy[i].y;
		}
	}
}
void platformCollision_Trzy(Person gracz, Platform platforms_three[PLATFORMS_THREE], bool* onPlatform, bool isJumping, bool* isFalling, double* distanceY, double* distanceX) {


	for (int i = 0; i < PLATFORMS_THREE; i++) {
		if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, platforms_three[i].rect)) {
			*onPlatform = true;
			if (isJumping) {
				*distanceY = 0;
			}
			else {
				*isFalling = false;
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_three[i], gracz, gracz.rect.x, gracz.rect.y + j)) { //bÍdzie siÍ obniøa≥o, kolizja boczna z lewej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			for (int j = 0; j < 24; j++) {
				if (checkCollisionAtPositionSide(platforms_three[i], gracz, gracz.rect.x + 24, gracz.rect.y + j)) { //bÍdzie siÍ obniøa≥o, kolizja boczna z prawej
					if (!isJumping) {
						*distanceX = 0;
						*onPlatform = false;
					}
					else if (isJumping && j > 1) {
						*distanceX = 0;
						*onPlatform = false;
					}
				}
			}
			break;
		}
	}
}
void collisionBoth_Trzy(Person gracz, Platform platforms_three[PLATFORMS_THREE], Ladder laddres_three[LADDERS_THREE], bool keyUp, bool* onBoth) {

	if (keyUp) {    // W G”RE
		for (int i = 0; i < PLATFORMS_THREE; i++) {
			for (int j = 0; j < LADDERS_THREE; j++) {
				if (checkCollisionAtPosition(platforms_three[i], laddres_three[j], gracz.rect.x, gracz.rect.y)) {
					*onBoth = true;
				}
			}
		}
	}
	else {   // W D”£
		for (int i = 0; i < PLATFORMS_THREE; i++) {
			for (int j = 0; j < LADDERS_THREE; j++) {
				if (checkCollisionAtPosition(platforms_three[i], laddres_three[j], gracz.rect.x + 12, gracz.rect.y + 24)) {
					*onBoth = true;
				}
			}
		}
	}
}


// MAIN
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {
	int t1, t2, quit, frames, rc;
	double delta, worldTime, fpsTimer, fps, distanceX, distanceY, moveSpeed, distanceBeczki;
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Surface* postac;
	SDL_Surface* drabina;
	SDL_Surface* flaga;
	SDL_Surface* ksiezniczka;
	SDL_Surface* beczka;
	SDL_Surface* serce;
	SDL_Surface* puste_serce;
	SDL_Surface* napis;

	printf("wyjscie printfa trafia do tego okienka\n");
	printf("printf output goes here\n");

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// tryb pe≥noekranowy / fullscreen mode
//	rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
//	                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
		&window, &renderer);
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Kacper Zuchowski - Proj. King Donkey");


	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);


	// wy≥πczenie widocznoúci kursora myszy
	//SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if (charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int bialy = SDL_MapRGB(screen->format, 254, 254, 254);
	int szary = SDL_MapRGB(screen->format, 160, 160, 160);


	// POSTAC	25x25
	postac = SDL_LoadBMP("./postac.bmp");
	if (postac == NULL) {
		printf("SDL_LoadBMP(postac.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	// DRABINA 25x70
	drabina = SDL_LoadBMP("./drabina.bmp");
	if (drabina == NULL) {
		printf("SDL_LoadBMP(drabina.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	// flaga 30x40
	flaga = SDL_LoadBMP("./flaga.bmp");
	if (flaga == NULL) {
		printf("SDL_LoadBMP(flaga.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	// ksiezniczka 30x40
	ksiezniczka = SDL_LoadBMP("./ksiezniczka.bmp");
	if (ksiezniczka == NULL) {
		printf("SDL_LoadBMP(ksiezniczka.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	// beczka 25x25
	beczka = SDL_LoadBMP("./beczka.bmp");
	if (beczka == NULL) {
		printf("SDL_LoadBMP(beczka.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	// serce 22x20
	serce = SDL_LoadBMP("./serce.bmp");
	if (serce == NULL) {
		printf("SDL_LoadBMP(serce.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};// puste-serce 20x20
	puste_serce= SDL_LoadBMP("./puste_serce.bmp");
	if (puste_serce== NULL) {
		printf("SDL_LoadBMP(puste_serce.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	napis = SDL_LoadBMP("./napis.bmp");
	if (napis == NULL) {
		printf("SDL_LoadBMP(napis.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	Person gracz = { POCZATKOWA_X_GR, POCZATKOWA_Y_GR,{POCZATKOWA_X_GR, POCZATKOWA_Y_GR, 25, 25} };	// zdef poczatkowa X i Y dla gr. przesuniÈcie na srodek wiec, x+13 y+12 dla wyúwietlania
	Person krolowa = { SCREEN_WIDTH - 450, SCREEN_HEIGHT - 440,{  SCREEN_WIDTH - 465, SCREEN_HEIGHT - 460, 30, 40 } };

	Barrel beczki[LICZBA_BECZEK] = {	// SKR”CI£EM WYSOKOå∆ O 5 PIKS ABY NIE ZACHACZA∆ O R”G 
		{ 100, SCREEN_HEIGHT - 380, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},
		{ 100, SCREEN_HEIGHT - 380, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},	// 2
		{ 100, SCREEN_HEIGHT - 380, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},
		{ 100, SCREEN_HEIGHT - 380, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},	// 4
		{ 100, SCREEN_HEIGHT - 380, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},
		{ 100, SCREEN_HEIGHT - 385 + 5, 1, {100, SCREEN_HEIGHT - 385 + 5, 25, 25 - 5}},	// 6
	};

	Barrel beczki_Dwa[LICZBA_BECZEK] = {
		{ SCREEN_WIDTH/2 - 20, SCREEN_HEIGHT - 400, 1,{ SCREEN_WIDTH / 2 - 20, SCREEN_HEIGHT - 400, 25, 25}},	// 1 w dÛ≥ / lewy gora
		{ 10, SCREEN_HEIGHT - 400, 1,{ 10, SCREEN_HEIGHT - 400, 25, 25}},	// 2 w lewo / lewy gÛra
		{ SCREEN_WIDTH - 65, SCREEN_HEIGHT - 400, 1, {  SCREEN_WIDTH - 65, SCREEN_HEIGHT - 400, 25, 25}},	// prawy gÛra	3
		{ PLATFORM_DIFF - 10, SCREEN_HEIGHT - 35, 1, { PLATFORM_DIFF - 10, SCREEN_HEIGHT - 35, 25, 25}},	// dol	4
		{ 0, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 55, 1, {0, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 55, 25, 25}},	// srodek	5	

	};

	Barrel beczki_Trzy[LICZBA_BECZEK] = {
		{ SCREEN_WIDTH - 50, SCREEN_HEIGHT - PLATFORM_DIFF - 35, 1, { SCREEN_WIDTH - 50, SCREEN_HEIGHT - PLATFORM_DIFF - 35, 25, 25}},
		{ SCREEN_WIDTH / 2 + 30, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 35, 1, { SCREEN_WIDTH / 2 + 30, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 35, 25, 25}},
		{ PLATFORM_DIFF - 20, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 35, 1, { PLATFORM_DIFF - 20, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 35, 25, 25}},
		{ SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 35, 1, { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 35, 25, 25}},
		{ SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 35, 1, { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 35, 25, 25}}
	};

	Ladder laddres_one[LADDERS_ONE] = {
		{{SCREEN_WIDTH - 112 , SCREEN_HEIGHT - 85, 25, 70}},// dla x -12 dla y -35
		{{88, SCREEN_HEIGHT - 155, 25, 70}},
		{{SCREEN_WIDTH - 112, SCREEN_HEIGHT - 225, 25, 70}},
		{{88, SCREEN_HEIGHT - 295, 25, 70}},
		{{SCREEN_WIDTH - 112, SCREEN_HEIGHT - 365, 25, 70}},
		{{SCREEN_WIDTH - 412, SCREEN_HEIGHT - 435, 25, 70}},
	};
		Ladder laddres_two[LADDERS_TWO] = {
			{{PLATFORM_DIFF + 38, SCREEN_HEIGHT - 85, 25, 70}},
			{{SCREEN_WIDTH / 2 - 22, SCREEN_HEIGHT - 155, 25, 70}},
			{{SCREEN_WIDTH - 222, SCREEN_HEIGHT - 225, 25, 70}},	
			{{168, SCREEN_HEIGHT - 295, 25, 70}},	// CHYBA TA DO PRZESUNIECIA |||
			{{SCREEN_WIDTH - 222, SCREEN_HEIGHT - 365, 25, 70}},
			{{SCREEN_WIDTH - 424, SCREEN_HEIGHT - 435, 25, 70}},
		};
			Ladder laddres_three[LADDERS_THREE] = {
				{{SCREEN_WIDTH - 82, SCREEN_HEIGHT - 85, 25, 70}},
				{{SCREEN_WIDTH / 2 + 8, SCREEN_HEIGHT - 155, 25, 70}},
				{{ 88, SCREEN_HEIGHT - 225, 25, 70}},
				{{SCREEN_WIDTH - 112, SCREEN_HEIGHT - 295, 25, 70}},
				{{SCREEN_WIDTH - 252, SCREEN_HEIGHT - 365, 25, 70}},
				{{SCREEN_WIDTH - 342, SCREEN_HEIGHT - 365, 25, 70}},
				{{SCREEN_WIDTH - 424, SCREEN_HEIGHT - 435, 25, 70}},
			};

	Platform platforms_one[PLATFORMS_ONE] = {

		{{0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10}},
		{{0, SCREEN_HEIGHT - PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF, 10}},
		{{PLATFORM_DIFF, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF, 10}},
		{{ 0, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF, 10}},
		{{PLATFORM_DIFF, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF, 10}},
		{{0, SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF, 10}},
		{{200, SCREEN_HEIGHT - 420, 100, 10}}
	};
		Platform platforms_two[PLATFORMS_TWO] = {

			{{0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10}},		
			{{100, SCREEN_HEIGHT - PLATFORM_DIFF - 10, 250, 10}},
			{{SCREEN_WIDTH - 80, SCREEN_HEIGHT - PLATFORM_DIFF - 10, 50, 10}},

			{{300, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 10, SCREEN_WIDTH / 2 - PLATFORM_DIFF, 10}},
				{{50, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 10, 50, 10}},

			{{ 150, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 10, 350, 10}},
			{{SCREEN_WIDTH - 80, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 10, 50, 10}},

			{{PLATFORM_DIFF, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 10, SCREEN_WIDTH - PLATFORM_DIFF - 100, 10}},
			{{PLATFORM_DIFF - 20, SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 10, 450, 10}},
			{{200, SCREEN_HEIGHT - 420, 100, 10}}
		};
			Platform platforms_three[PLATFORMS_THREE] = {

				{{0, SCREEN_HEIGHT - 10, SCREEN_WIDTH, 10}},		
				{{330, SCREEN_HEIGHT - PLATFORM_DIFF - 10, 300, 10}},
				{{20, SCREEN_HEIGHT - PLATFORM_DIFF - 10, 100, 10}},

				{{80, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 10, SCREEN_WIDTH / 2 - 40, 10}},
				{{SCREEN_WIDTH - 70, SCREEN_HEIGHT - 2 * PLATFORM_DIFF - 10, 50, 10}},

				{{10, SCREEN_HEIGHT - 3 * PLATFORM_DIFF - 10, SCREEN_WIDTH - 80, 10}},

				{{SCREEN_WIDTH / 2 + 80, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 10, SCREEN_WIDTH / 2 - 120, 10}},
				{{230, SCREEN_HEIGHT - 4 * PLATFORM_DIFF - 10, SCREEN_WIDTH / 2 - 200, 10}},
				{{PLATFORM_DIFF + 130, SCREEN_HEIGHT - 5 * PLATFORM_DIFF - 10, 270, 10}},
				{{200, SCREEN_HEIGHT - 420, 100, 10}}
			};

	Obszar pole[5] = {
		{ SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 34, 100, 17, false},
		{ SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 14, 100, 17, false},
		{ SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 5, 100, 17, false},
		{ SCREEN_WIDTH / 4, SCREEN_HEIGHT / 2 + 26, 340, 17, false},
		{ SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 46, 100, 17, false},

	};

	t1 = SDL_GetTicks();

	frames = 0;
	fpsTimer = 0;
	fps = 0;
	quit = 0;
	worldTime = 0;
	distanceX = 0;
	distanceY = 0;
	moveSpeed = 0.00;
	float V_POZ = 160.0, V_PION = PREDKOSC_PIONOWA, V_GRAWITACJA = PREDKOSC_GRAWITACYJNA, jumpHeight = 0.0;
	int ilosc_beczek = 0, opoznienie_beczki = 1, ZYCIA=3, NR_ETAPU = 0;

	bool keyStates[SDL_NUM_SCANCODES] = { false }; // Tablica stanÛw klawiszy
	bool isJumping = false, isFalling = false, onLadder = false, onPlatform = false, onBoth = false, changeLvl = false, dead = false;
	
	while (!quit) {

		t2 = SDL_GetTicks();

		int time_to_wait = TARGET_TIME - (SDL_GetTicks() - t1);

		if (time_to_wait > 0 && time_to_wait <= TARGET_TIME) {	// ograniczenie liczby klatek
			SDL_Delay(time_to_wait);
		}

		delta = (t2 - t1) * 0.001f;
		t1 = t2;
		worldTime += delta;
		fpsTimer += delta;
		if (fpsTimer >= 0.5f) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		};

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				keyStates[event.key.keysym.scancode] = true;
				break;
			case SDL_KEYUP:
				keyStates[event.key.keysym.scancode] = false;
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_MOUSEMOTION:
				// Sprawdü, czy mysz jest nad obszarem
				for (int i = 0; i < 5; i++) {
					pole[i].mysz = czyMyszNadObszarem(event.motion.x, event.motion.y, &pole[i]);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				// Sprawdü, czy klikniÍcie myszy nastπpi≥o w obszarze
					if (event.button.button == SDL_BUTTON_LEFT && pole[0].mysz==true) {
						NR_ETAPU = 1;
						restartEtapu(&worldTime, &gracz);
					}
					if (event.button.button == SDL_BUTTON_LEFT && pole[1].mysz == true) {
						NR_ETAPU = 2;
						restartEtapu(&worldTime, &gracz);
					}
					if (event.button.button == SDL_BUTTON_LEFT && pole[2].mysz == true) {
						NR_ETAPU = 3;
						restartEtapu(&worldTime, &gracz);
					}
					if (event.button.button == SDL_BUTTON_LEFT && pole[4].mysz == true) {
						quit = 1;
					}
				break;
			}
		}

		// Aktualizacja stanÛw klawiszy
		bool keyLeft = keyStates[SDL_SCANCODE_LEFT];
		bool keyRight = keyStates[SDL_SCANCODE_RIGHT];
		bool keyUp = keyStates[SDL_SCANCODE_UP];
		bool keyDown = keyStates[SDL_SCANCODE_DOWN];
		bool keySpace = keyStates[SDL_SCANCODE_SPACE];
		bool keyN = keyStates[SDL_SCANCODE_N];
		bool keyESC = keyStates[SDL_SCANCODE_ESCAPE];
		bool keyEnter = keyStates[SDL_SCANCODE_RETURN];
		bool keyJeden = keyStates[SDL_SCANCODE_1];	// ETAP 1
		bool keyDwa = keyStates[SDL_SCANCODE_2];	// ETAP 2
		bool keyTrzy = keyStates[SDL_SCANCODE_3];	// ETAP 3
		bool keyMENU = keyStates[SDL_SCANCODE_0];	// MENU , ETAP 0


		if (keyESC) {
			quit = 1;
		}
		if (keyJeden) {
			NR_ETAPU = 1;
			ilosc_beczek = 0;
			opoznienie_beczki = 1;
			for (int i = 0; i < LICZBA_BECZEK; i++) {
				beczki[i].pkt_kontrolny = 1;
				beczki[i].x = 100;
				beczki[i].y = SCREEN_HEIGHT - 380;

				beczki[i].rect.x = beczki[i].x;
				beczki[i].rect.y = beczki[i].y;
			}
			restartEtapu(&worldTime, &gracz);
		}
		if (keyDwa) {
			NR_ETAPU = 2;
			restartEtapu(&worldTime, &gracz);
		}
		if (keyTrzy) {
			NR_ETAPU = 3;
			restartEtapu(&worldTime, &gracz);
		}
		if (keyMENU) {
			NR_ETAPU = 0;
			restartEtapu(&worldTime, &gracz);
		}
		// MENU 
		if (NR_ETAPU == 0) {

			SDL_ShowCursor(SDL_ENABLE);	// MYSZKA

			SDL_FillRect(screen, NULL, szary);
			for (int i = 0; i < 5; i++) {
				if (pole[i].mysz) {
					DrawRectangle(screen, pole[i].x, pole[i].y, pole[i].w, pole[i].h, czarny, czarny);
				}
			}
			DrawSurface(screen, napis, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100);
			
			sprintf(text, " Etap 1 ");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT/2 - 30, text, charset);
			sprintf(text, " Etap 2 ");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 10, text, charset);
			sprintf(text, " Etap 3 ");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 10 , text, charset);
			if (pole[3].mysz == true) {
				sprintf(text, " Sprawdzenie wynikow (Opcja niedostepna) ");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 30, text, charset);
			}
			else {
				sprintf(text, " Sprawdzenie wynikow");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 30, text, charset);
			}
			sprintf(text, " Wyjscie ");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 + 50, text, charset);
			
			ZYCIA = 3;
		}
		else {

			SDL_ShowCursor(SDL_DISABLE);
			if (dead) {
				if (keyN) {	
					ZYCIA -= 1;
					dead = false;
					if (ZYCIA == 0) {
						NR_ETAPU = 0;
					}
					restartEtapu(&worldTime, &gracz);
				}
			}
			else {
				if (keyN) {
					restartEtapu(&worldTime, &gracz);	
				}
				if (keyEnter && changeLvl) {
					changeLvl = false;
					NR_ETAPU += 1;
					if (NR_ETAPU > LICZBA_ETAPOW) {	
						NR_ETAPU = 0;
					}
					restartEtapu(&worldTime, &gracz);
				}
				if (keyLeft) {
					moveSpeed = V_POZ;
					distanceX = -moveSpeed * delta;
				}
				else if (keyRight) {
					moveSpeed = V_POZ;
					distanceX = moveSpeed * delta;
				}
				else {
					distanceX = 0;
				}

				if (!onLadder && !onPlatform) {
					moveSpeed = V_GRAWITACJA;
					distanceY = moveSpeed * delta;
				}
				else {
					if (keyUp && onLadder) {
						moveSpeed = V_PION; 
						distanceY = -moveSpeed * delta;
					}
					else if (keyDown) {
						moveSpeed = V_PION; 
						distanceY = moveSpeed * delta;
					}
					else if (!onPlatform) {
						moveSpeed = V_GRAWITACJA;
						distanceY = moveSpeed * delta;
					}
				}

				if (keySpace) {
					if (!isJumping && !isFalling) {
						isJumping = true;
						jumpHeight = 0.0;
					}
				}
				if (isJumping) {
					jumpHeight += 250.0 * delta;

					if (jumpHeight >= 90.0){

						isJumping = false;
						isFalling = true;
					}
					else {
						distanceY = -250.0 * delta;
					}
				}
			}
			// BECZKI
			distanceBeczki = 120.0 * delta;

			// POSTAC, podstawowe/sztywne ograniczenie ruchu plansza
			if ((gracz.rect.x + distanceX) <= 0 || (gracz.rect.x + distanceX) >= (SCREEN_WIDTH - 25)) {
				distanceX = 0;
				if ((gracz.rect.y + distanceY) >= (SCREEN_HEIGHT - 22) || (gracz.rect.y + distanceY) <= 52) {
					distanceY = 0;
					gracz.rect.y = (SCREEN_HEIGHT - 23);
				}
			}
			else {
				if ((gracz.rect.y + distanceY) >= (SCREEN_HEIGHT - 22) || (gracz.rect.y + distanceY) <= 52) {
					distanceY = 0;
					gracz.rect.y = (SCREEN_HEIGHT - 23);
				}
			}

			gracz.rect.x += distanceX;
			gracz.rect.y += distanceY;
			onLadder = false;
			onPlatform = false;
			bool onBoth = false;

			SDL_FillRect(screen, NULL, czarny);
			// OBRAM”WKA
			DrawLine(screen, 0, 51, SCREEN_WIDTH, 1, 0, zielony);	// pozioma gora

			if (NR_ETAPU == 1) {

				rysujPierwszy(screen, platforms_one, drabina, flaga, ksiezniczka, beczka, zielony, czerwony);
				// Mechanika beczek (ruch i wyúwietlanie)
				if (worldTime >= opoznienie_beczki) {

					opoznienie_beczki = 5 + worldTime;
					if (ilosc_beczek < LICZBA_BECZEK) {
						ilosc_beczek += 1;
					}
				}
				Beczki_Jeden(beczki, dead, &ilosc_beczek, distanceBeczki, screen, beczka, zielony, &opoznienie_beczki);

				platformCollision_Jeden(gracz, platforms_one, &onPlatform, isJumping, &isFalling, &distanceY, &distanceX, onLadder);
				for (int j = 0; j < LADDERS_ONE; j++) {
					if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, laddres_one[j].rect)) {
						onLadder = true;
						break;
					}
				}
				collisionBoth_Jeden(gracz, platforms_one, laddres_one, keyUp, &onBoth);

			}
			else if (NR_ETAPU == 2) {
				rysujDrugi(screen, platforms_two, drabina, flaga, ksiezniczka, zielony, czerwony);
				// BECZKI
				Beczki_Dwa(beczki_Dwa, dead, &ilosc_beczek, distanceBeczki, screen, beczka, zielony, &opoznienie_beczki);

				platformCollision_Dwa(gracz, platforms_two, &onPlatform, isJumping, &isFalling, &distanceY, &distanceX);
				for (int j = 0; j < LADDERS_TWO; j++) {
					if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, laddres_two[j].rect)) {
						onLadder = true;
						break;
					}
				}
				collisionBoth_Dwa(gracz, platforms_two, laddres_two, keyUp, &onBoth);

			}
			else {	// 3
				rysujTrzeci(screen, platforms_three, drabina, flaga, ksiezniczka, zielony, czerwony);
				// MECHANIKA BECZEK
				Beczki_Trzy(beczki_Trzy, dead, &ilosc_beczek, distanceBeczki, screen, beczka, zielony, &opoznienie_beczki);
				platformCollision_Trzy(gracz, platforms_three, &onPlatform, isJumping, &isFalling, &distanceY, &distanceX);
				for (int j = 0; j < LADDERS_THREE; j++) {
					if (checkCollision({ gracz.rect.x, gracz.rect.y, gracz.rect.w, gracz.rect.h }, laddres_three[j].rect)) {
						onLadder = true;
						break;
					}
				}
				collisionBoth_Trzy(gracz, platforms_three, laddres_three, keyUp, &onBoth);
			}


			if (onLadder) {

				if (onBoth) {
					if (moveSpeed == V_GRAWITACJA) {
						distanceY = 0;
					}
					if (!onPlatform) {
						onBoth = false;
					}
				}
				else {
					if (!onPlatform) {
						if (moveSpeed == V_GRAWITACJA) {
							gracz.rect.y -= distanceY;
							distanceY = 0;
						}
					}
					else {
						if (!keyUp) {
							distanceY = 0;
						}
					}
				}
			}
			else if (onPlatform) {
				distanceY = 0.0;
			}
			else { // onPlatform == false && onLadder == false
				if (isJumping == false && moveSpeed != V_GRAWITACJA) {
					gracz.rect.y -= distanceY;
					distanceY = 0;
				}
			}

			// udane przejúÊie etapu / Gry
			if (checkCollision(gracz.rect, krolowa.rect)) {
				if (NR_ETAPU == LICZBA_ETAPOW) {
					sprintf(text, " Gratulacje! Udalo ci sie przejsc gre! Nacisnij (ENTER) aby wrocic do menu.");
					DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
					changeLvl = true;
				}
				else {
					sprintf(text, " Gratulacje! Przeszedles poziom %d, Nacisnij (ENTER) aby przejsc na nastepny poziom!", NR_ETAPU);
					DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
					changeLvl = true;
				}
			}

			// ZDERZENIE Z BECZKA, åMIERC
			for (int i = 0; i < LICZBA_BECZEK; i++) {
				if (NR_ETAPU == 1) {
					if (checkCollision(gracz.rect, beczki[i].rect)) {
						distanceX = 0;
						distanceY = 0;
						dead = true;
					}
				}
				else if (NR_ETAPU == 2) {
					if (checkCollision(gracz.rect, beczki_Dwa[i].rect)) {
						distanceX = 0;
						distanceY = 0;
						dead = true;
					}
				}
				else {
					if (checkCollision(gracz.rect, beczki_Trzy[i].rect)) {
						distanceX = 0;
						distanceY = 0;
						dead = true;
					}
				}
			}
			if (ZYCIA == 3) {
				DrawSurface(screen, serce, 24, 74);
				DrawSurface(screen, serce, 54, 74);
				DrawSurface(screen, serce, 84, 74);
			}
			else if (ZYCIA == 2) {
				DrawSurface(screen, serce, 24, 74);
				DrawSurface(screen, serce, 54, 74);
				DrawSurface(screen, puste_serce, 82, 74);
			}
			else if (ZYCIA == 1) {
				DrawSurface(screen, serce, 24, 74);
				DrawSurface(screen, puste_serce, 52, 74);
				DrawSurface(screen, puste_serce, 80, 74);
			}
			if (dead) {
				sprintf(text, " !! Nie zyjesz, zacznij ponownie !! (Nacisnij: n )");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, charset);
			}

			gracz.x += distanceX;
			gracz.y += distanceY;

			DrawSurface(screen, postac, gracz.x + 12, gracz.y + 11);	// przesuniÈcie na srodek wiec x+12,  y+11

			gracz.rect.x = gracz.x;	// aktualizacja w stosunku do wyúwietlania
			gracz.rect.y = gracz.y;

			DrawRectangle(screen, 1, 1, SCREEN_WIDTH - 2, 50, czerwony, szary);
			sprintf(text, " Numer etapu - % d    Czas = %.1lf s", NR_ETAPU, worldTime);
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);

			sprintf(text, " Esc - wyjscie, (0) - Menu, (n) - restart etapu,  %.0lf klatek/s ", fps);
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 24, text, charset);

			sprintf(text, "(SPACJA)-skok  \032 -w lewo  \033 -w prawo  \30 -w gore  \31 -w dol");
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 40, text, charset);

		}
		

		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);

		frames++;
	};	// Koniec petli gry

	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};
