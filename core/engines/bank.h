/*
 * OpenVALive - the sound bank: every sound a track can take.
 *
 * A "sound" is a number 0..OLS_SOUND_COUNT-1: first the VA presets, then the
 * drum kits. Each track owns one engine of each kind (a slot); choosing a
 * sound loads it into the right engine and, if the kind changes, plugs that
 * engine into the track (ols_live_set_engine). Hosts (FM-1, desktop) use
 * this so the list of sounds is the same everywhere.
 */
#ifndef OLS_BANK_H
#define OLS_BANK_H

#include "va/engine_va.h"
#include "drum/engine_drum.h"
#include "../live/live.h"

enum { OLS_KIND_VA = 0, OLS_KIND_DRUM = 1, OLS_KIND_COUNT };

#define OLS_SOUND_COUNT (VA_PRESET_COUNT + DR_KIT_COUNT)
#define OLS_SOUND_FIRST_DRUM VA_PRESET_COUNT
#define OLS_USER_SOUNDS 8
#define OLS_SOUND_FIRST_USER OLS_SOUND_COUNT
#define OLS_SOUND_TOTAL (OLS_SOUND_COUNT + OLS_USER_SOUNDS)   /* factory sounds, then USER 1..8 */

/* A sound kept by the player: the engine kind and every parameter (124 bytes). */
typedef struct {
    uint8_t valid;
    uint8_t kind;
    uint16_t v[OLS_MAX_PARAMS];
} ols_user_sound_t;

/* The engines one track can switch between. */
typedef struct {
    engine_va_t *va;
    engine_drum_t *drum;
} ols_slot_t;

const char *ols_sound_name(int sound);       /* "PAD", "DRUMS 808"... */
int ols_sound_kind(int sound);
ols_engine_t ols_slot_engine(ols_slot_t *slot, int kind);

/* Audio side: loads a sound into a track (engine switch included). Sounds
 * from OLS_SOUND_FIRST_USER on come from `user` (may be 0: then ignored). */
void ols_bank_select(ols_live_t *L, ols_slot_t *slots, int track, int sound);
void ols_bank_select_user(ols_live_t *L, ols_slot_t *slots, int track, const ols_user_sound_t *user);

/* Audio side: keeps the track's sound as it is now into `user`. */
void ols_bank_store_user(const ols_live_t *L, int track, ols_user_sound_t *user);

/* Lets project files switch engines when a saved track used another kind. */
void ols_bank_attach(ols_live_t *L, ols_slot_t *slots);

#endif /* OLS_BANK_H */
