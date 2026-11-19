/*
 * OpenVALive - FM-1: the DSP unit, compiled at -O2 (fm1.toml: o2), apart
 * from the UI which fm1-base compiles at -Os.
 * The DSP code lives in ../../core (portable, shared with every target) and
 * is included here so that fm1-base builds it as one unit.
 */
#include "ols_config.h"

#include "../core/engines/va/engine_va.c"
#include "../core/engines/va/engine_va_params.c"
#include "../core/engines/va/engine_va_presets.c"
#include "../core/engines/va/engine_va_voices.c"
#include "../core/engines/va/engine_va_render.c"
#include "../core/engines/va/engine_va_mod.c"
#include "../core/engines/drum/engine_drum.c"
#include "../core/engines/drum/engine_drum_voices.c"
#include "../core/engines/bank.c"
#include "../core/live/midi_in.c"

/* the Live Core: clock, sequencer, capture, undo, scenes, Creative Engine, projects */
#include "../core/live/pattern.c"
#include "../core/live/clock.c"
#include "../core/live/creative.c"
#include "../core/live/live.c"
#include "../core/live/live_record.c"
#include "../core/live/live_history.c"
#include "../core/live/live_scenes.c"
#include "../core/live/live_cmd.c"
#include "../core/live/project.c"
