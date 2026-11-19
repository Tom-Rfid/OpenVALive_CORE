# 06 — API et connecteurs : utiliser le DSP ailleurs

Le cœur d'OpenVALive est une bibliothèque C99 sans dépendance : pas de matériel, pas d'allocation, pas de libc
obligatoire. Trois niveaux d'utilisation.

## Niveau 1 : l'API C (le plus simple)

```c
#include "core/api/ols_synth.h"

static int16_t delay_ram[16384];          /* 341 ms à 48 kHz, ou 0 pour pas de delay */
static ols_synth_t synth;

void setup(void)
{
    ols_synth_init(&synth, 48000.0f, delay_ram, 16384);
    ols_synth_load_preset(&synth, VA_PRESET_PAD);
}

void on_midi_bytes(const uint8_t *b, int n) { ols_synth_midi(&synth, b, n); }

void audio_callback(float *left, float *right, int frames)
{
    ols_synth_render_f32(&synth, left, right, frames);
}
```

Fichiers à compiler :
```
core/api/ols_synth.c
core/engines/va/engine_va*.c        (5 fichiers)
core/live/midi_in.c
core/dsp/dsp_runtime.c              (fréquence d'échantillonnage réglable)
```
Includes : la racine du dépôt (les chemins sont relatifs). Lien : `-lm` sur PC.

| Fonction | Rôle |
|---|---|
| `ols_synth_init(s, sr, delay, len)` | init ; `sr` en Hz |
| `ols_synth_midi(s, octets, n)` | octets MIDI bruts (running status géré, SysEx ignoré) |
| `ols_synth_usb_midi(s, paquet)` | paquet USB-MIDI de 4 octets |
| `ols_synth_note_on / note_off / all_notes_off` | notes directes |
| `ols_synth_set_param(s, id, v01)` | paramètre normalisé ; `id` = `va_param_t` |
| `ols_synth_param_count / param_name` | pour construire une UI automatiquement |
| `ols_synth_load_preset(s, VA_PRESET_*)` | INIT, BASS, PAD, LEAD |
| `ols_synth_map_cc(s, cc, id)` | associer un CC à un paramètre |
| `ols_synth_render_f32(s, l, r, n)` | rendu, sortie [−1, 1] |
| `ols_synth_render_i16(s, lr, n, scratch)` | rendu entrelacé int16 pour I2S |

Exemple complet et testé : [`connectors/c-api/example.c`](../../connectors/c-api/example.c) (`make build/api_example`).

### Règles

- **Fil d'exécution** : tout appeler depuis le fil audio, ou protéger les appels (Teensy : `AudioNoInterrupts()`),
  ou passer par `core/live/event_queue.h` (file SPSC sans verrou, comme le FM-1 et l'application bureau).
- **Fréquence d'échantillonnage** : une par programme, réglée par `ols_synth_init`.
- **Taille des blocs** : libre. Le moteur découpe en sous-blocs de 32.
- **RAM** : `ols_synth_t` ≈ 3,4 Ko + le buffer de delay fourni.
- **CPU** (indicatif) : ~670 instructions x86 par échantillon pour 4 voix + chorus + delay ; ~5 % d'un pi32v2 à
  360 MHz par voix (mesuré sur FM-1).

## Niveau 2 : le moteur directement

`core/engines/va/engine_va.h` donne accès à tout : `engine_va_init`, `note_on`, `set_param`, `render`,
`voice_limit`… L'interface générique `core/engines/engine.h` (`ols_engine_api_t`) permet d'écrire du code qui
pilote n'importe quel futur moteur (Acid, FM…) sans le connaître.

## Niveau 3 : les briques DSP

Chaque bloc de `core/dsp/` est un en-tête autonome, en fonctions `static inline` :

```c
#include "core/dsp/osc.h"
#include "core/dsp/svf.h"

dsp_osc_t o;  osc_init(&o, 0.0f);  osc_set_freq(&o, 110.0f);
dsp_svf_t f;  svf_reset(&f);       svf_set(&f, 800.0f, 0.6f);   /* à chaque bloc */

for (int i = 0; i < n; i++) {
    osc_advance(&o);
    out[i] = svf_process(&f, osc_value(&o, OSC_SAW), SVF_LP);
}
```

## Les connecteurs du dépôt

| Connecteur | État | Notes |
|---|---|---|
| `connectors/desktop` | testé (CI Linux, .exe Windows sous Wine) | jeu live, MIDI + clavier d'ordinateur ([13](13-application-bureau.md)) |
| `connectors/c-api` | testé | écrit un WAV à 44,1 kHz |
| `connectors/pc-sim` | testé | démo, balayage d'aliasing, mesure CPU |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) · [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | les appareils, dans leurs propres dépôts | fm1-base · smk37_open |
| `connectors/daisy-seed` | **non testé** | libDaisy, MIDI UART, delay en SDRAM |
| `connectors/teensy` | **non testé** | objet `AudioStream`, usbMIDI |

## Idées de portage

| Cible | Piste |
|---|---|
| Plugin VST3/CLAP/AU | JUCE : `prepareToPlay` → `ols_synth_init(sampleRate)` ; `processBlock` → le MIDI du buffer puis `render_f32` sur les canaux 0 et 1. |
| RP2040 / RP2350 | I2S par PIO ; la RP2350 a une FPU simple précision. Sur RP2040 (sans FPU), prévoir moins de voix. |
| ESP32-S3 | I2S natif, FPU simple précision ; rendre dans une tâche épinglée sur un cœur. |
| Web Audio | WebAssembly (emscripten) + `AudioWorklet`. fm1-base fait déjà tourner l'app FM-1 entière dans le navigateur ([14](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_FR/14-simulateur-web-et-site.md)). |

Un portage qui marche mérite une PR : dossier `connectors/<cible>/`, un README avec les commandes, et « testé sur … ».

## Le Live Core et le moteur batterie (V0.2)

`core/api/ols_synth.h` reste le moyen le plus simple d'avoir un synthé. Pour l'instrument entier (quatre pistes,
boucles, CAPTURE, scènes, Creative Engine, batterie), utiliser `core/live/live.h` avec `core/engines/bank.h`.
L'application bureau (`connectors/desktop/engine_bridge.c`) en est un exemple complet et court, et
[16 — Le Live Core](16-le-live-core.md) montre la mise en place en quelques lignes. Le moteur batterie seul
(`core/engines/drum/engine_drum.h`) est un `ols_engine_t` comme le VA : n'importe quel hôte peut l'utiliser.
