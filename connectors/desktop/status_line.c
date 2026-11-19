/*
 * OpenVALive desktop - one status line, rewritten in place:
 *   PLAY 120 | T2 BASS  step  5/16  4 notes | >CUTOFF 55 | voices 3/32 | CPU 4% | MIDI: SMK-37 | CAPTURE
 */
#include <stdio.h>
#include "desktop.h"

const char *last_message(void);

static const char *transport(void)
{
    if (app.live.recording) {
        return "REC ";
    }
    if (app.live.clock.running) {
        return "PLAY";
    }
    return app.live.clock.external ? "EXT " : "STOP";
}

void status_line_print(void)
{
    const ols_track_t *tr = &app.live.track[app.track];
    unsigned steps = tr->pat.length / OLS_TICKS_PER_STEP;

    printf("\r%s %3d | T%d %-11s%s step %2u/%-3u %2u notes | %s %3d | voices %2d | CPU %2d%% | MIDI: %s | %-28s",
           transport(), (int)(clock_bpm(&app.live.clock) + 0.5f),
           app.track + 1, ols_sound_name(app.preset[app.track]), tr->mute ? " (muted)" : "",
           (unsigned)ols_live_step(&app.live, app.track) + 1u, steps, tr->pat.count,
           knob_name(app.selected_knob), (int)(desktop_knob_value(app.selected_knob) * 100.0f + 0.5f),
           app.voices_sounding, (int)(app.cpu_load * 100.0f + 0.5f),
           app.midi_name ? app.midi_name : "none", last_message());
    fflush(stdout);
}
