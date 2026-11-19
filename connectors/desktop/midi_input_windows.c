/*
 * OpenVALive desktop - MIDI input on Windows (WinMM, built into Windows).
 * Bytes received in WinMM's callback thread are pushed into app.midi_bytes.
 */
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#include "desktop.h"

static HMIDIIN handle;
static char device_name[MAXPNAMELEN];

/* number of bytes in a short MIDI message, from its status byte */
static int message_length(uint8_t status)
{
    if (status >= 0xF8) return 1;          /* real-time */
    switch (status & 0xF0) {
    case 0xC0:
    case 0xD0: return 2;
    default:   return 3;
    }
}

static void CALLBACK on_midi(HMIDIIN in, UINT msg, DWORD_PTR user, DWORD_PTR p1, DWORD_PTR p2)
{
    (void)in; (void)user; (void)p2;
    if (msg != MIM_DATA) {
        return;
    }
    uint8_t status = (uint8_t)(p1 & 0xFF);
    int n = message_length(status);
    byte_ring_push(&app.midi_bytes, status);
    if (n > 1) byte_ring_push(&app.midi_bytes, (uint8_t)((p1 >> 8) & 0x7F));
    if (n > 2) byte_ring_push(&app.midi_bytes, (uint8_t)((p1 >> 16) & 0x7F));
}

void midi_input_list(void)
{
    UINT count = midiInGetNumDevs();
    printf("MIDI inputs: %u\n", count);
    for (UINT i = 0; i < count; i++) {
        MIDIINCAPSA caps;
        if (midiInGetDevCapsA(i, &caps, sizeof caps) == MMSYSERR_NOERROR) {
            printf("  %u: %s\n", i, caps.szPname);
        }
    }
}

int midi_input_open(const char *which)
{
    UINT count = midiInGetNumDevs();
    if (count == 0) {
        return 0;
    }
    UINT index = which ? (UINT)atoi(which) : 0;
    if (index >= count) {
        fprintf(stderr, "No MIDI input %u (see --list).\n", index);
        return 0;
    }
    MIDIINCAPSA caps;
    midiInGetDevCapsA(index, &caps, sizeof caps);
    if (midiInOpen(&handle, index, (DWORD_PTR)on_midi, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
        fprintf(stderr, "MIDI input \"%s\" is busy (close the other MIDI programs).\n", caps.szPname);
        return 0;
    }
    midiInStart(handle);
    lstrcpynA(device_name, caps.szPname, sizeof device_name);
    app.midi_name = device_name;
    return 1;
}

void midi_input_close(void)
{
    if (handle) {
        midiInStop(handle);
        midiInClose(handle);
        handle = 0;
    }
}
#endif /* _WIN32 */
