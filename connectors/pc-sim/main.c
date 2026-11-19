/*
 * OpenVALive - PC simulator, command line.
 *
 *   ols_sim demo  out.wav    16-bar demo: mono bass + 4-voice pad
 *   ols_sim sweep out.wav    saw sweep 50 Hz -> 15 kHz (aliasing check)
 *   ols_sim bench            CPU cost per case, static RAM
 */
#include <stdio.h>
#include <string.h>
#include "sim.h"

int main(int argc, char **argv)
{
    dsp_set_sample_rate((float)SIM_SR);

    if (argc >= 2 && strcmp(argv[1], "bench") == 0) {
        sim_bench();
        return 0;
    }
    if (argc >= 3 && strcmp(argv[1], "sweep") == 0) {
        return sim_sweep(argv[2]);
    }
    if (argc >= 3 && strcmp(argv[1], "demo") == 0) {
        return sim_demo(argv[2]);
    }
    fprintf(stderr, "usage: %s demo out.wav | sweep out.wav | bench\n", argv[0]);
    return 1;
}
