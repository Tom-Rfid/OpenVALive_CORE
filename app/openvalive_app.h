/*
 * OpenVALive app: what every target shares (the FM-1, the SMK-37, a fork). A target's own app.h includes this file
 * and adds its name, its splash colour, where its flash objects live (APP_STORE_MAP) and its platform switches
 * (APP_TOUCH, APP_OVERLOAD ...). See app/README.md.
 */
#pragma once
#include <stdint.h>
#include "plat.h"                   /* the platform: fm1-base's plat.h, or a target's own implementation of it */

#define APP_BLOCK 256u              /* frames per audio call at most (the FM-1: 5.8 ms) */
#define APP_CANVAS_PX (240u * 80u)  /* the tallest band drawn at once (ui_draw.c: the tracks, 4 x 20 rows) */

/* what the app keeps in flash: settings, the platform's panel object (the FM-1's calibration; a target without one
 * stores nothing there), the live set (sounds, loops, scenes) in two objects of up to 3840 bytes, the 8 user sounds,
 * the MAP CONTROL */
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_PROJECT_A, OBJ_PROJECT_B, OBJ_USER_SOUNDS, OBJ_MAP, OBJ_NOBJ };

/* the buttons with a role of their own (the FM-1's labels): SEL holds the tracks layer (mutes, clear, clock...),
 * takes a waiting sound and, held with SELECT, points at a line (from A: swaps); SEQ holds the perform layer (loops,
 * filter, breaks); FX turns the knob pages. A target maps its buttons onto these. */
#ifndef BTN_TRACKS                     /* (a target whose platform names the buttons otherwise sets its own) */
#define BTN_TRACKS B_SEL
#define BTN_VALIDATE B_SEL
#define BTN_PAGES B_FX
#endif
#define BTN_PERFORM B_SEQ

/* called by the platform */
void app_init(void);                                            /* boot, before audio starts */
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);  /* audio interrupt: n stereo Q23 frames */
void app_panic(void);                                           /* silence now (before a flash update) */
void app_ui_init(void);                                         /* first screen after the splash */
void app_frame(void);                                           /* main loop, ~60 Hz */
void app_input_only(void);                                      /* between frames: input only */
void app_say(const char *a, const char *b);                     /* short message (and screen reader) */
void app_overload(void);                                        /* CPU overload: shed a voice */
