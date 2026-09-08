#ifndef SAVE_H
#define SAVE_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "settings.h"

enum { SAVE_SIZE = 64, SAVE_MAGIC = 0x42524B31u, SAVE_VERSION = 1, SAVE_MAX_GAMES = 8 };
typedef struct { settings_t settings; int32_t high_scores[SAVE_MAX_GAMES]; } save_t;
void save_defaults(save_t *s);
void save_encode(const save_t *s, uint8_t out[SAVE_SIZE]);          /* big-endian fields, CRC32 (IEEE, poly 0xEDB88320) over bytes 0..59 stored at 60..63 */
bool save_decode(const uint8_t in[SAVE_SIZE], save_t *out);         /* false (and defaults) on bad magic, version, or CRC */
uint32_t save_crc32(const uint8_t *data, size_t len);

#endif
