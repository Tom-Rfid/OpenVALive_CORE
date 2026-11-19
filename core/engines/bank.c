/*
 * OpenVALive - the sound bank, see bank.h.
 */
#include "bank.h"

const char *ols_sound_name(int sound)
{
    static const char *const USER_NAMES[OLS_USER_SOUNDS] = {
        "USER 1", "USER 2", "USER 3", "USER 4", "USER 5", "USER 6", "USER 7", "USER 8",
    };
    if (sound >= OLS_SOUND_FIRST_USER && sound < OLS_SOUND_TOTAL) {
        return USER_NAMES[sound - OLS_SOUND_FIRST_USER];
    }
    static const char *const DRUM_NAMES[DR_KIT_COUNT] = { "DRUMS 808", "DRUMS 909", "DRUMS TIGHT", "DRUMS LOFI", "DRUMS IRON" };
    if (sound < 0 || sound >= OLS_SOUND_COUNT) {
        return "?";
    }
    if (sound < OLS_SOUND_FIRST_DRUM) {
        return VA_PRESET_NAMES[sound];
    }
    return DRUM_NAMES[sound - OLS_SOUND_FIRST_DRUM];
}

int ols_sound_kind(int sound)
{
    /* (a user sound's kind is in the sound itself: ols_user_sound_t.kind) */
    return sound >= OLS_SOUND_FIRST_DRUM ? OLS_KIND_DRUM : OLS_KIND_VA;
}

ols_engine_t ols_slot_engine(ols_slot_t *slot, int kind)
{
    ols_engine_t e;
    if (kind == OLS_KIND_DRUM) {
        e.api = &ENGINE_DRUM_API;
        e.ctx = slot->drum;
    } else {
        e.api = &ENGINE_VA_API;
        e.ctx = slot->va;
    }
    return e;
}

void ols_bank_select(ols_live_t *L, ols_slot_t *slots, int track, int sound)
{
    if (track < 0 || track >= L->track_count || sound < 0 || sound >= OLS_SOUND_COUNT) {
        return;
    }
    ols_slot_t *slot = &slots[track];
    int kind = ols_sound_kind(sound);

    if (kind == OLS_KIND_DRUM) {
        engine_drum_all_notes_off(slot->drum);
        engine_drum_load_kit(slot->drum, (dr_kit_t)(sound - OLS_SOUND_FIRST_DRUM));
    } else {
        engine_va_all_notes_off(slot->va);
        engine_va_load_preset(slot->va, (va_preset_t)sound);
    }
    if (L->track[track].kind != kind) {
        ols_live_set_engine(L, track, ols_slot_engine(slot, kind), (uint8_t)kind);
    }
}

/* Called by ols_project_load for a track saved with another kind of engine. */
void ols_bank_store_user(const ols_live_t *L, int track, ols_user_sound_t *user)
{
    const ols_track_t *tr = &L->track[track];
    uint16_t count = tr->engine.api->param_count;
    for (uint16_t id = 0; id < OLS_MAX_PARAMS; id++) {
        float v = id < count ? tr->engine.api->get_param(tr->engine.ctx, id) : 0.0f;
        user->v[id] = (uint16_t)(v <= 0.0f ? 0.0f : (v >= 1.0f ? 65535.0f : v * 65535.0f + 0.5f));
    }
    user->kind = tr->kind;
    user->valid = 1;
}

void ols_bank_select_user(ols_live_t *L, ols_slot_t *slots, int track, const ols_user_sound_t *user)
{
    if (user == 0 || !user->valid || track < 0 || track >= L->track_count || user->kind >= OLS_KIND_COUNT) {
        return;
    }
    ols_slot_t *slot = &slots[track];
    ols_engine_t engine = ols_slot_engine(slot, user->kind);
    engine.api->all_notes_off(engine.ctx);
    for (uint16_t id = 0; id < engine.api->param_count && id < OLS_MAX_PARAMS; id++) {
        engine.api->set_param(engine.ctx, id, (float)user->v[id] * (1.0f / 65535.0f));
    }
    if (user->kind == OLS_KIND_VA) {
        engine_va_snap(slot->va);                  /* a new sound starts exactly where it should */
    }
    if (L->track[track].kind != user->kind) {
        ols_live_set_engine(L, track, engine, user->kind);
    }
}

static ols_engine_t engine_for_kind(void *ctx, int track, uint8_t kind)
{
    ols_slot_t *slots = (ols_slot_t *)ctx;
    return ols_slot_engine(&slots[track], kind);
}

void ols_bank_attach(ols_live_t *L, ols_slot_t *slots)
{
    L->kind_ctx = slots;
    L->engine_for_kind = engine_for_kind;
}
