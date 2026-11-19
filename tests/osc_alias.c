/* Renders a fixed PolyBLEP oscillator as raw float32 on stdout, for analysis.
 *   osc_alias FREQ WAVE(0 saw, 1 square, 2 triangle) > out.f32 */
#define DSP_SAMPLE_RATE 48000
#include <stdio.h>
#include <stdlib.h>
#include "../core/dsp/osc.h"
int main(int c, char **v){ float f=atof(v[1]); int w=atoi(v[2]); dsp_osc_t o; osc_init(&o,0); osc_set_freq(&o,f);
 for(int i=0;i<8192;i++){ osc_advance(&o); float s=osc_value(&o,(osc_wave_t)w); fwrite(&s,4,1,stdout);} return 0;}
