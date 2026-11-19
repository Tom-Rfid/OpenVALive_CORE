/*
 * OpenVALive - Live Core
 * project.h : a whole live set as bytes (to save in flash, in a file, or send).
 *
 * Contents: tempo, swing, quantize, the selected track; per track its mute,
 * gain, every engine parameter and its pattern; the stored scenes.
 *
 * Format "OLSP" version 1, little endian, ends with a CRC-32. Parameters are
 * stored as 16-bit values (0..65535 = 0..1). A loader refuses anything
 * damaged, and a project made for another number of tracks or parameters
 * loads what fits. Size: about 1 KB per busy track plus 0.4 KB per scene,
 * at most OLS_PROJECT_MAX bytes.
 */
#ifndef OLS_PROJECT_H
#define OLS_PROJECT_H

#include "live.h"

#define OLS_PROJECT_MAX (16u + OLS_TRACKS * (8u + 2u * OLS_MAX_PARAMS + 8u * OLS_PATTERN_NOTES) \
                         + 1u + OLS_SCENES * (2u + OLS_TRACKS * 2u * OLS_MAX_PARAMS) + 4u)

/* Writes the project into buf. Returns its size, or 0 if max is too small. */
uint32_t ols_project_save(const ols_live_t *L, uint8_t *buf, uint32_t max);

/* Loads a project. Returns 1 on success; 0 (and nothing changed) if the data is damaged.
 * Call it on the audio side, or before audio starts. */
int ols_project_load(ols_live_t *L, const uint8_t *buf, uint32_t len);

uint32_t ols_crc32(const uint8_t *data, uint32_t len);

#endif /* OLS_PROJECT_H */
