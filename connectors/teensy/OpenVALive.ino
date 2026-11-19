/*
 * OpenVALive on Teensy 4.x (Teensy Audio Library)  --  UNTESTED TEMPLATE
 *
 * Setup: copy the repository's core/ folder into a src/ folder next to this
 * sketch (Arduino compiles src/ recursively), then choose
 * "USB Type: MIDI + Audio" or "MIDI" in the Tools menu.
 * Only reviewed, never compiled: corrections welcome.
 */
#include <Audio.h>
extern "C" {
#include "src/core/api/ols_synth.h"
}

static ols_synth_t synth;
static int16_t delay_ram[16384];                       /* 371 ms at 44.1 kHz */

/* A Teensy Audio Library object that renders OpenVALive */
class AudioSynthOpenVALive : public AudioStream {
public:
    AudioSynthOpenVALive() : AudioStream(0, NULL) {}
    void update(void) override {
        audio_block_t *l = allocate(), *r = allocate();
        if (!l || !r) { if (l) release(l); if (r) release(r); return; }
        static float fl[AUDIO_BLOCK_SAMPLES], fr[AUDIO_BLOCK_SAMPLES];
        ols_synth_render_f32(&synth, fl, fr, AUDIO_BLOCK_SAMPLES);
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            l->data[i] = (int16_t)(constrain(fl[i], -1.0f, 1.0f) * 32767.0f);
            r->data[i] = (int16_t)(constrain(fr[i], -1.0f, 1.0f) * 32767.0f);
        }
        transmit(l, 0); transmit(r, 1);
        release(l); release(r);
    }
};

AudioSynthOpenVALive  ols;
AudioOutputI2S      out;                               /* Audio Shield; or AudioOutputUSB */
AudioConnection     c1(ols, 0, out, 0), c2(ols, 1, out, 1);
AudioControlSGTL5000 shield;

/* usbMIDI arrives in loop(); update() runs in an interrupt: protect the calls */
static void onNoteOn(byte ch, byte note, byte vel) { AudioNoInterrupts(); ols_synth_note_on(&synth, note, vel); AudioInterrupts(); }
static void onNoteOff(byte ch, byte note, byte vel) { AudioNoInterrupts(); ols_synth_note_off(&synth, note); AudioInterrupts(); }
static void onCC(byte ch, byte cc, byte v)
{
    uint8_t m[3] = { (uint8_t)(0xB0 | ((ch - 1) & 15)), cc, v };
    AudioNoInterrupts(); ols_synth_midi(&synth, m, 3); AudioInterrupts();
}

void setup()
{
    AudioMemory(8);
    ols_synth_init(&synth, AUDIO_SAMPLE_RATE_EXACT, delay_ram, sizeof delay_ram / sizeof delay_ram[0]);
    ols_synth_load_preset(&synth, VA_PRESET_BASS);
    shield.enable();
    shield.volume(0.6f);
    usbMIDI.setHandleNoteOn(onNoteOn);
    usbMIDI.setHandleNoteOff(onNoteOff);
    usbMIDI.setHandleControlChange(onCC);
}

void loop()
{
    usbMIDI.read();
}
