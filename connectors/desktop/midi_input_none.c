/*
 * OpenVALive desktop - no MIDI input yet on this OS (macOS: CoreMIDI to do).
 * The computer keyboard still plays the synth.
 */
#if !defined(_WIN32) && !defined(__linux__)
#include <stdio.h>
#include "desktop.h"

void midi_input_list(void)
{
    printf("MIDI input is not supported on this system yet (contributions welcome: CoreMIDI).\n");
}

int midi_input_open(const char *which)
{
    (void)which;
    return 0;
}

void midi_input_close(void) {}
#endif
