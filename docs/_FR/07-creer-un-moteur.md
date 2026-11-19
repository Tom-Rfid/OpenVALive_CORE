# 07 — Créer un moteur (instrument)

Un moteur est un instrument : VA aujourd'hui, Acid, FM, Drum demain. Il réutilise le DSP Core et s'expose par
l'interface commune `core/engines/engine.h`. Le séquenceur, les scènes, le MIDI et l'UI ne connaissent que cette
interface.

## L'interface

```c
typedef struct {
    const char *name;
    uint16_t param_count;
    const char *const *param_names;            /* noms courts (≤ 8 caractères) */
    void  (*note_on)(void *ctx, uint8_t note, uint8_t velocity);
    void  (*note_off)(void *ctx, uint8_t note);
    void  (*all_notes_off)(void *ctx);
    void  (*set_param)(void *ctx, uint16_t id, float value01);
    float (*get_param)(void *ctx, uint16_t id);
    void  (*pitch_bend)(void *ctx, float semitones);
    void  (*render)(void *ctx, float *out_l, float *out_r, int n);
} ols_engine_api_t;
```

## Étapes

1. **Dossier** `core/engines/<nom>/`. Reprendre l'organisation de `engines/va` : un en-tête public, puis un
   fichier par rôle (api, params, presets, voices, render) et un `_internal.h` privé.
2. **Paramètres** : un `enum` d'identifiants, des valeurs **normalisées 0..1** dans `p[]`, et toutes les
   conversions (Hz, secondes, demi-tons) dans `set_param`, rangées dans une structure « dérivée ». Jamais de
   conversion dans `render`.
3. **Rendu** : boucle par sous-blocs de `DSP_BLOCK` ; modulations en début de sous-bloc, échantillons dans la
   boucle interne. Copier ce dont la boucle a besoin dans des variables locales `const`.
4. **Mémoire** : aucune allocation. Les gros buffers sont fournis par l'appelant (`PLAT_POOL` sur FM-1).
5. **Table** `const ols_engine_api_t ENGINE_<NOM>_API = { ... };`.
6. **Presets** : au moins INIT et un son qui montre le caractère du moteur.
7. **Fichiers de build** : ajouter les `.c` dans `app/ols_dsp.c` (FM-1) et dans `CORE_SRC` du Makefile
   et de `tests/run_tests.sh`.
8. **Tests** : ajouter un cas dans `tests/run_tests.sh` (pas de NaN, pas d'écrêtage, justesse) et un rendu de démo.
9. **Budget** : mesurer avec `make bench` (adapter `connectors/pc-sim/bench.c`) puis sur le FM-1 (CPU à l'écran).
   Visé : moins de 10 % du FM-1 par voix, moins de 50 Ko de flash par moteur.

## Règles DSP du projet

- `float` uniquement, suffixe `f` partout (`0.5f`) ; le build FM-1 refuse tout `double`.
- Pas de `tanf`/`expf`/`powf` à la fréquence audio ; utiliser les `dsp_*` de `dsp_common.h`.
- Pas de `<string.h>`, pas de `NULL` (écrire `0`), pas de `malloc`.
- Le moteur ne lit ni boutons ni MIDI : il reçoit des appels.
- Lisible avant d'être malin : une fonction par rôle, commentaires en anglais qui expliquent le *pourquoi*.

## Pistes pour les prochains moteurs

| Moteur | À étudier | Licence de la référence |
|---|---|---|
| Acid (303) | Open303, x0xb0x, AcidBox | à vérifier pour chacun avant d'en reprendre du code |
| FM | Dexed (msfa) | Dexed GPL-3 ; msfa Apache-2.0 |
| Drum | Felucca `drum_voice.c` | GPL-3 : s'inspirer, ne pas copier |
| divers | Felucca-Engines (ANALOG, LOFI, TRIO, WHEEL, NOISE, PHASE) | **MIT** : réutilisable avec la mention |
| divers | Mutable Instruments (Plaits, Braids, Elements) | MIT |

Avant d'importer du code externe, lire [09 — Licences](09-licences.md).
