/*
 * OpenVALive - FM-1 app: audio side (runs in the audio interrupt).
 *
 * Every 5.8 ms fm1-base asks for 256 stereo frames:
 *   1. apply the events pushed by the UI,
 *   2. read incoming MIDI (USB, TRS): notes go to the selected track, clock to the Live Core,
 *   3. render the Live Core (it cuts the block at every sequencer tick),
 *   4. convert to the codec format (Q23 in int32) and feed the oscilloscope.
 * Nothing here may block: no storage, no screen.
 */

/* ------------------------------------------------------------ A: the live voice and its arpeggiator */

/* A's sound: a synth preset, or a user sound that holds a synth (audio side, or at boot) */
static void live_voice_sound(uint32_t sound)
{
    engine_va_all_notes_off(&lv);
    if (sound >= OLS_SOUND_FIRST_USER) {
        const ols_user_sound_t *u = &user_sounds[(sound - OLS_SOUND_FIRST_USER) % OLS_USER_SOUNDS];
        if (!u->valid || u->kind != OLS_KIND_VA) {
            return;
        }
        for (uint16_t id = 0; id < ENGINE_VA_API.param_count && id < OLS_MAX_PARAMS; id++) {
            engine_va_set_param(&lv, id, (float)u->v[id] * (1.0f / 65535.0f));
        }
        engine_va_snap(&lv);
    } else if (sound < OLS_SOUND_FIRST_DRUM) {
        engine_va_load_preset(&lv, (va_preset_t)sound);
    }
    engine_va_set_voice_limit(&lv, LIVE_VOICES);
}

static void arp_silence(void)
{
    if (arp.sounding) {
        engine_va_note_off(&lv, arp.sounding);
        arp.sounding = 0;
    }
    arp.gate_open = 0;
}

static void arp_key_down(uint8_t note, uint8_t vel)
{
    for (uint32_t i = 0; i < arp.count; i++) {
        if (arp.note[i] == note) {
            return;
        }
    }
    if (arp.count < ARP_HELD) {
        arp.note[arp.count] = note;
        arp.vel[arp.count] = vel;
        arp.count++;
    }
    if (arp.count == 1u) {
        arp.phase = 1.0f;                       /* the first key plays at once */
        arp.pos = 0xFFu;
        arp.dir = 1;
    }
}

static void arp_key_up(uint8_t note)
{
    for (uint32_t i = 0; i < arp.count; i++) {
        if (arp.note[i] == note) {
            for (uint32_t j = i + 1u; j < arp.count; j++) {
                arp.note[j - 1u] = arp.note[j];
                arp.vel[j - 1u] = arp.vel[j];
            }
            arp.count--;
            break;
        }
    }
    if (arp.count == 0u) {
        arp_silence();
    }
}

/* the next note of the arpeggio: the held keys from low to high, in the mode's order */
static uint32_t arp_next(uint8_t *vel)
{
    uint8_t order[ARP_HELD], n = arp.count;
    for (uint32_t i = 0; i < n; i++) {              /* the held keys sorted (8 at most) */
        order[i] = (uint8_t)i;
        for (uint32_t j = i; j > 0u && arp.note[order[j]] < arp.note[order[j - 1u]]; j--) {
            uint8_t s = order[j];
            order[j] = order[j - 1u];
            order[j - 1u] = s;
        }
    }
    uint32_t p = arp.pos;
    switch (arp_mode) {
    case ARP_DOWN:
        p = p >= n ? n - 1u : (p + n - 1u) % n;
        break;
    case ARP_UPDOWN:
        if (p >= n) {
            p = 0;
            arp.dir = 1;
        } else if (n > 1u) {
            if ((arp.dir > 0 && p + 1u >= n) || (arp.dir < 0 && p == 0u)) {
                arp.dir = (int8_t)-arp.dir;
            }
            p = arp.dir > 0 ? p + 1u : p - 1u;
        }
        break;
    case ARP_RANDOM:
        arp.rng = arp.rng * 1664525u + 1013904223u;
        p = (arp.rng >> 24) % n;
        break;
    default:                                    /* UP */
        p = p >= n ? 0u : (p + 1u) % n;
        break;
    }
    arp.pos = (uint8_t)p;
    *vel = arp.vel[order[p]];
    return arp.note[order[p]];
}

/* the arpeggio over the next n samples: a note on every 16th of the tempo, held half of it */
static void arp_advance(uint32_t n)
{
    if (arp_mode == ARP_OFF || arp.count == 0u) {
        if (arp.sounding) {
            arp_silence();
        }
        return;
    }
    float step = clock_bpm(&live.clock) * (4.0f / 60.0f) / DSP_SR_F;   /* 16ths per sample */
    arp.phase += step * (float)n;
    if (arp.gate_open && arp.phase >= 0.5f && arp.phase < 1.0f) {
        arp_silence();
    }
    if (arp.phase >= 1.0f) {
        arp.phase -= arp.phase >= 2.0f ? arp.phase : 1.0f;
        arp_silence();
        uint8_t vel;
        arp.sounding = (uint8_t)arp_next(&vel);
        engine_va_note_on(&lv, arp.sounding, vel);
        arp.gate_open = 1;
    }
}

static void live_voice_event(const ols_event_t *e)
{
    switch (e->type) {
    case EV_NOTE_ON:
        if (arp_mode != ARP_OFF) {
            arp_key_down(e->a, e->b);
        } else {
            engine_va_note_on(&lv, e->a, e->b);
        }
        break;
    case EV_NOTE_OFF:
        arp_key_up(e->a);                       /* (harmless when the key was not arpeggiated) */
        if (arp_mode == ARP_OFF) {
            engine_va_note_off(&lv, e->a);
        }
        break;
    case EV_PARAM:
        if (e->id < ENGINE_VA_API.param_count) {
            engine_va_set_param(&lv, e->id, e->v);
        }
        break;
    case EV_PRESSURE:
        ENGINE_VA_API.pressure(&lv, e->v);
        break;
    case EV_PRESET:
        if (e->b == 1u) {                       /* b = 1: keep A's sound as user sound a, with A's octave */
            if (e->a < OLS_USER_SOUNDS) {
                ols_user_sound_t *u = &user_sounds[e->a];
                for (uint16_t id = 0; id < OLS_MAX_PARAMS; id++) {
                    float v = id < ENGINE_VA_API.param_count ? engine_va_get_param(&lv, id) : 0.0f;
                    u->v[id] = (uint16_t)(v <= 0.0f ? 0.0f : (v >= 1.0f ? 65535.0f : v * 65535.0f + 0.5f));
                }
                u->kind = OLS_KIND_VA;
                u->valid = USER_VALID((int8_t)e->v);
            }
            break;
        }
        arp.count = 0;
        arp_silence();
        live_voice_sound(e->a);
        break;
    default:
        break;
    }
}

/* A's voice into the mix, by chunks (the arpeggio's steps land within 1.5 ms) */
static void live_voice_render(uint32_t n)
{
    float g = lv_gain;
    for (uint32_t done = 0; done < n;) {
        uint32_t c = n - done < (uint32_t)OLS_RENDER_CHUNK ? n - done : (uint32_t)OLS_RENDER_CHUNK;
        arp_advance(c);
        ENGINE_VA_API.render(&lv, live.tmp_l, live.tmp_r, (int)c);
        for (uint32_t i = 0; i < c; i++) {
            buf_l[done + i] += live.tmp_l[i] * g;
            buf_r[done + i] += live.tmp_r[i] * g;
        }
        done += c;
    }
}

/* PERFORM SLICER: the whole mix opened and shut in 1/16s on the clock (the beat when playing, the tempo when
 * stopped), each edge smoothed over ~2 ms (no click); let go, it opens back smoothly */
static void slicer_render(uint32_t n)
{
    static float g = 1.0f;
    if (!slicer_on && g > 0.999f) {
        g = 1.0f;
        return;
    }
    uint32_t tick = live.clock.running ? live.clock.tick : live.clock.free_tick;
    float target = (!slicer_on || (tick % 24u) < 12u) ? 1.0f : 0.05f;
    for (uint32_t i = 0; i < n; i++) {
        g += (target - g) * 0.01f;
        buf_l[i] *= g;
        buf_r[i] *= g;
    }
}

/* The master EQ (EQ page): the knobs are bytes of the settings the UI writes (one store each, no tearing); the
 * filters follow when one moves (control rate), and cost nothing while all is flat. */
static eq3_t master_eq = { .flat = 1 };
static uint8_t eq_applied[EQ_BANDS] = { 128, 128, 128, 128 };

static void master_eq_render(uint32_t n)
{
    int moved = 0;
    for (uint32_t b = 0; b < EQ_BANDS; b++) {
        moved |= set.eq[b] != eq_applied[b];
        eq_applied[b] = set.eq[b];
    }
    if (moved) {
        eq3_set(&master_eq, (float)eq_applied[0] * (1.0f / 255.0f), (float)eq_applied[1] * (1.0f / 255.0f),
                (float)eq_applied[2] * (1.0f / 255.0f), (float)eq_applied[3] * (1.0f / 255.0f), (float)DSP_SAMPLE_RATE);
    }
    eq3_process(&master_eq, buf_l, buf_r, n);
}

/* A <-> a synth track: every parameter of the two sounds (filter, envelopes, delay, chorus...) changes place */
static void swap_with_a(int t)
{
    if (t < 0 || t >= TRACKS || live.track[t].kind != OLS_KIND_VA) {
        return;
    }
    arp_silence();
    engine_va_all_notes_off(&lv);
    engine_va_all_notes_off(&va[t]);
    for (uint16_t id = 0; id < ENGINE_VA_API.param_count && id < OLS_MAX_PARAMS; id++) {
        float a = engine_va_get_param(&lv, id);
        engine_va_set_param(&lv, id, engine_va_get_param(&va[t], id));
        engine_va_set_param(&va[t], id, a);
    }
    engine_va_snap(&lv);
    engine_va_snap(&va[t]);
    engine_va_set_voice_limit(&lv, LIVE_VOICES);
    engine_va_set_voice_limit(&va[t], VOICE_LIMIT[t]);
}

static void apply_event(const ols_event_t *e)
{
#ifdef APP_CLIPS
    if (e->type == EV_CLIP) {
        clip_event(e);
        return;
    }
    if (e->type == EV_NOTE_ON || e->type == EV_NOTE_OFF) {    /* a take listens to its line */
        clip_note_seen(e->target, e->a, e->type == EV_NOTE_ON ? e->b : 0u);
    }
#endif
    if (e->target == T_LIVE) {
        live_voice_event(e);
        return;
    }
#ifdef APP_CLIPS
    if (e->target == T_PADS) {
        pad_drum_event(e);
        return;
    }
    if (e->type == EV_ALL_OFF) {
        ENGINE_DRUM_API.all_notes_off(&pad_drum);
    }
#endif
    if (e->type == EV_ALL_OFF) {                /* panic: A too */
        arp.count = 0;
        arp_silence();
        engine_va_all_notes_off(&lv);
    }
    if (e->type == EV_PRESET) {
        /* sounds belong to the bank (synth presets, drum kits, user sounds), not to the Live Core.
         * b = 1: keep the track's sound as user sound a */
        int t = e->target < TRACKS ? e->target : live.current;
        if (e->b == 2u) {                       /* b = 2: swap the track's sound with A's */
            swap_with_a(t);
            return;
        }
        if (e->b == 1u) {
            if (e->a < OLS_USER_SOUNDS) {
                ols_bank_store_user(&live, t, &user_sounds[e->a]);
                user_sounds[e->a].valid = USER_VALID((int8_t)e->v);     /* the track's octave goes with it */
            }
        } else if (e->a >= OLS_SOUND_FIRST_USER) {
            ols_bank_select_user(&live, slots, t, &user_sounds[(e->a - OLS_SOUND_FIRST_USER) % OLS_USER_SOUNDS]);
        } else {
            ols_bank_select(&live, slots, t, e->a);
        }
        engine_va_set_voice_limit(&va[t], VOICE_LIMIT[t]);
        return;
    }
    ols_live_apply_event(&live, e);
}

static void apply_pending_events(void)
{
    ols_event_t e;
    while (evq_pop(&evq, &e)) {
        apply_event(&e);
    }
}

/* fm1-base delivers USB-MIDI packets as one 32-bit word: CIN, status, data1, data2. */
static void read_midi_input(void)
{
    uint32_t packet;
    int port;
    uint32_t now24 = plat_ticks24();

    while (plat_midi_in(&packet, &port)) {
        /* how long ago it landed (fm1.toml midi_stamp): one sample is 544 ticks of 24 MHz.
         * The clock follower measures the tempo between real arrivals: no block jitter. */
        uint32_t ago = (now24 - plat_midi_in_t24()) / 544u;
        live.midi_late = ago < (uint32_t)APP_BLOCK * 4u ? ago : 0u;
        uint8_t bytes[4];
        bytes[0] = (uint8_t)packet;
        bytes[1] = (uint8_t)(packet >> 8);
        bytes[2] = (uint8_t)(packet >> 16);
        bytes[3] = (uint8_t)(packet >> 24);
        midi_in_usb_packet(&midi, bytes);
    }
    live.midi_late = 0;
}

/* The Live Core's MIDI out (sequenced notes, clock): over USB, the FM-1 has no MIDI OUT jack. */
static void usb_midi_out(void *ctx, uint8_t status, uint8_t d1, uint8_t d2)
{
    (void)ctx;
    uint32_t cin = status >= 0xF8u ? 0x0Fu : (uint32_t)(status >> 4);
    plat_midi_send(cin | (uint32_t)status << 8 | (uint32_t)d1 << 16 | (uint32_t)d2 << 24, TO_USB);
}

/* float [-1, 1] -> Q23 stereo, with the MASTER gain smoothed per sample (no clicks). */
static void write_output(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    /* full scale of a Q23 sample is 8388607 */
    float gain_target = (float)master_q12 * (1.0f / 4096.0f) * 8388000.0f;
    uint32_t w = scope_write;

    const int speaker = on_speaker;

    for (uint32_t i = 0; i < n; i++) {
        gain_now += (gain_target - gain_now) * 0.004f;

        float l = buf_l[i];
        float r = buf_r[i];
        if (speaker) {
            l = svf_process(&speaker_cut[0], l, SVF_HP);
            r = svf_process(&speaker_cut[1], r, SVF_HP);
        }
        l = fm_clampf(l, -1.0f, 1.0f) * gain_now;
        r = fm_clampf(r, -1.0f, 1.0f) * gain_now;
        lr[2 * i] = (int32_t)l;
        lr[2 * i + 1] = (int32_t)r;

        scope_phase++;
        if (scope_phase >= SCOPE_DECIMATE) {
            scope_phase = 0;
            scope[w & (SCOPE_N - 1u)] = (int16_t)(buf_l[i] * 32767.0f);
            w++;
        }
    }
    scope_write = w;
}

/* The saved live set, once the UI said go (see app_ui_init). */
static void load_pending_project(void)
{
    if (project_pending != 1u) {
        return;
    }
    if (ols_project_load(&live, project_buf + 2, project_size)) {
        for (int t = 0; t < TRACKS; t++) {
            engine_va_snap(&va[t]);
            engine_va_set_voice_limit(&va[t], VOICE_LIMIT[t]);
        }
    }
    saved_changes = live.changes;
    project_pending = 0;
}

void app_render(int32_t *lr, uint32_t n, uint32_t master_q12)
{
    load_pending_project();
    apply_pending_events();
    read_midi_input();
#ifdef APP_CLIPS
    clips_advance();
#endif

    if (n > APP_BLOCK) {
        n = APP_BLOCK;
    }
    ols_live_render(&live, buf_l, buf_r, (int)n);
    live_voice_render(n);
#ifdef APP_CLIPS
    for (uint32_t done = 0; done < n;) {        /* the pads' drum kit */
        uint32_t c = n - done < (uint32_t)OLS_RENDER_CHUNK ? n - done : (uint32_t)OLS_RENDER_CHUNK;
        ENGINE_DRUM_API.render(&pad_drum, live.tmp_l, live.tmp_r, (int)c);
        for (uint32_t i = 0; i < c; i++) {
            buf_l[done + i] += live.tmp_l[i];
            buf_r[done + i] += live.tmp_r[i];
        }
        done += c;
    }
#endif
    slicer_render(n);
    master_eq_render(n);
    write_output(lr, n, master_q12);

    audio_clock += n;
}

/* Called by fm1-base before an update rewrites the flash: silence now. */
void app_panic(void)
{
    ols_live_stop(&live);
    ols_live_all_notes_off(&live);
    arp.count = 0;
    arp_silence();
    engine_va_all_notes_off(&lv);
}

/* Called by fm1-base when rendering takes over 90 % of the time: the busiest track drops a voice. */
void app_overload(void)
{
    int busiest = 0;
    for (int t = 1; t < TRACKS; t++) {
        if (va[t].voice_limit > va[busiest].voice_limit) {
            busiest = t;
        }
    }
    if (va[busiest].voice_limit > 1) {
        engine_va_set_voice_limit(&va[busiest], va[busiest].voice_limit - 1);
    }
}
