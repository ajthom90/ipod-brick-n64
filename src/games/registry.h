#ifndef REGISTRY_H
#define REGISTRY_H
#include "game.h"
extern const game_desc_t *const GAMES[];
extern const int GAME_COUNT;
int game_index_by_name(const char *name);   /* case-insensitive, -1 if unknown */
#endif
