#ifndef SFX_H
#define SFX_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SFX_NONE = 0, SFX_BOUNCE, SFX_HIT, SFX_CLEAR, SFX_FOOD, SFX_SHOT, SFX_EXPLODE,
    SFX_POINT, SFX_MERGE, SFX_GAME_OVER, SFX_MENU_MOVE, SFX_MENU_SELECT, SFX_COUNT,
} sfx_id_t;

typedef struct { uint8_t ids[8]; uint8_t head, tail; } sfx_queue_t;   /* ring of up to 7 */

static inline void sfx_push(sfx_queue_t *q, sfx_id_t id) {
    uint8_t next = (uint8_t)((q->tail + 1) % 8);
    if (next == q->head) return;              /* full: drop */
    q->ids[q->tail] = (uint8_t)id; q->tail = next;
}

static inline sfx_id_t sfx_pop(sfx_queue_t *q) {
    if (q->head == q->tail) return SFX_NONE;
    sfx_id_t id = (sfx_id_t)q->ids[q->head]; q->head = (uint8_t)((q->head + 1) % 8);
    return id;
}

#endif
